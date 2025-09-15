/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <rcl/rcl.h>
#include <rclc/executor.h>

#include <micro_ros_utilities/string_utilities.h>
#include <std_msgs/msg/int32.h>

#include <microros_transports.h>
#include <rmw_microros/rmw_microros.h>

/* ============================================================================
 * CONFIGURATION AND CONSTANTS
 * ============================================================================
 */

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#if (DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, dac) &&            \
	 DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, dac_channel_id) && \
	 DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, dac_resolution))
#define DAC_NODE DT_PHANDLE(ZEPHYR_USER_NODE, dac)
#define DAC_CHANNEL_ID DT_PROP(ZEPHYR_USER_NODE, dac_channel_id)
#define DAC_RESOLUTION DT_PROP(ZEPHYR_USER_NODE, dac_resolution)
#else
#error "Unsupported board: see README and check /zephyr,user node"
#endif

#define NODE_NAME "zephyr_dac_controller"
#define NODE_NAMESPACE ""
#define TOPIC_NAME_MAX_LEN 32
#define TOPIC_NAME "dac/aout" STRINGIFY(DAC_CHANNEL_ID) "/raw"
#define DOMAIN_ID 0

/* Error Checking Macros */
#define RCCHECK(fn)                                                       \
	{                                                                     \
		rcl_ret_t temp_rc = fn;                                           \
		if ((temp_rc != RCL_RET_OK))                                      \
		{                                                                 \
			printf("Failed status on line %d: %d. Aborting.\n", __LINE__, \
				   (int)temp_rc);                                         \
			return 1;                                                     \
		}                                                                 \
	}
#define RCSOFTCHECK(fn)                                                     \
	{                                                                       \
		rcl_ret_t temp_rc = fn;                                             \
		if ((temp_rc != RCL_RET_OK))                                        \
		{                                                                   \
			printf("Failed status on line %d: %d. Continuing.\n", __LINE__, \
				   (int)temp_rc);                                           \
		}                                                                   \
	}

#define EXECUTE_EVERY_N_MS(MS, X)          \
	do                                     \
	{                                      \
		static volatile int64_t init = -1; \
		if (init == -1)                    \
		{                                  \
			init = uxr_millis();           \
		}                                  \
		if (uxr_millis() - init > MS)      \
		{                                  \
			X;                             \
			init = uxr_millis();           \
		}                                  \
	} while (0)

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================
 */

enum states
{
	WAITING_AGENT,
	AGENT_AVAILABLE,
	AGENT_CONNECTED,
	AGENT_DISCONNECTED
} state;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================
 */

rclc_support_t support;
rcl_init_options_t init_options;
rcl_node_t node;
rclc_executor_t executor;
rcl_allocator_t allocator;

rcl_subscription_t subscriber_int32;
std_msgs__msg__Int32 msg_int32;

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct device *const dac_dev = DEVICE_DT_GET(DAC_NODE);
static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id = DAC_CHANNEL_ID, .resolution = DAC_RESOLUTION, .buffered = true};

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */
static bool dac_initialize(void);
static bool create_entities(void);
static bool destroy_entities(void);
static void subscription_callback(const void *);

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================
 */
int main(void)
{
	rmw_uros_set_custom_transport(MICRO_ROS_FRAMING_REQUIRED, (void *)&default_params,
								  zephyr_transport_open, zephyr_transport_close,
								  zephyr_transport_write, zephyr_transport_read);

	// HW initialization
	if (!dac_initialize())
	{
		return -EIO;
	}

	// agent discovery state machine
	state = WAITING_AGENT;
	while (1)
	{
		switch (state)
		{
		case WAITING_AGENT:
			EXECUTE_EVERY_N_MS(500, state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1))
												? AGENT_AVAILABLE
												: WAITING_AGENT;);
			break;
		case AGENT_AVAILABLE:
			state = (true == create_entities()) ? AGENT_CONNECTED : WAITING_AGENT;
			if (state == WAITING_AGENT)
			{
				destroy_entities();
			}
			break;
		case AGENT_CONNECTED:
			// main execution loop - blocking call
			rclc_executor_spin(&executor);
			state = AGENT_DISCONNECTED;
			break;
		case AGENT_DISCONNECTED:
			destroy_entities();
			state = WAITING_AGENT;
			break;
		default:
			break;
		}
	}

	return 0;
} // main

/* ============================================================================
 * FUNCTION IMPLEMENTATIONS
 * ============================================================================
 */

static bool dac_initialize(void)
{
	int ret;

	if (!device_is_ready(dac_dev))
	{
		return false;
	}

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	if (ret < 0)
	{
		return false;
	}
	return true;
}

static bool create_entities(void)
{
	allocator = rcl_get_default_allocator();

	// create init options
	init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));
	RCCHECK(rcl_init_options_set_domain_id(&init_options, DOMAIN_ID))
	RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

	// create node
	RCCHECK(rclc_node_init_default(&node, NODE_NAME, NODE_NAMESPACE, &support));

	// create subscriber
	RCCHECK(rclc_subscription_init_default(&subscriber_int32, &node,
										   ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
										   TOPIC_NAME));

	// create executor
	executor = rclc_executor_get_zero_initialized_executor();
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscriber_int32, &msg_int32,
										   &subscription_callback, ON_NEW_DATA));

	return true;
}

static bool destroy_entities(void)
{
	rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
	(void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

	RCSOFTCHECK(rcl_subscription_fini(&subscriber_int32, &node));
	RCSOFTCHECK(rclc_executor_fini(&executor));
	RCSOFTCHECK(rcl_node_fini(&node));
	RCSOFTCHECK(rclc_support_fini(&support));
	RCSOFTCHECK(rcl_node_options_fini(&init_options));
}

static void subscription_callback(const void *msgin)
{
	const std_msgs__msg__Int32 *msg = (const std_msgs__msg__Int32 *)msgin;
	if ((msg->data < 0) || (msg->data >= (1 << DAC_RESOLUTION)))
	{
		return;
	}
	dac_write_value(dac_dev, DAC_CHANNEL_ID, msg->data);
}