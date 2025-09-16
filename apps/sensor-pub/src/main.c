/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include <stdio.h>

#include <rcl/rcl.h>
#include <rclc/executor.h>

#include <geometry_msgs/msg/point32.h>
#include <micro_ros_utilities/string_utilities.h>
#include <std_msgs/msg/int32.h>

#include <microros_transports.h>
#include <rmw_microros/rmw_microros.h>

/* ============================================================================
 * CONFIGURATION AND CONSTANTS
 * ============================================================================
 */

#define ACCEL_NODE DT_ALIAS(accel0)

#define NODE_NAME "zephyr_imu_node"
#define NODE_NAMESPACE CONFIG_TOPIC_PREFIX
#define TOPIC_NAME_MAX_LEN 32
#define TOPIC_NAME "imu"
#define DOMAIN_ID 0

/* Timing Configuration */
#define PUBLISH_PERIOD_MS CONFIG_TOPIC_PUBLISH_PERIOD_MS

/* Error Checking Macros */
#define RCCHECK(fn)                                                                   \
	{                                                                             \
		rcl_ret_t temp_rc = fn;                                               \
		if ((temp_rc != RCL_RET_OK))                                          \
		{                                                                     \
			printf("Failed status on line %d: %d. Aborting.\n", __LINE__, \
			       (int)temp_rc);                                         \
			return 1;                                                     \
		}                                                                     \
	}
#define RCSOFTCHECK(fn)                                                                 \
	{                                                                               \
		rcl_ret_t temp_rc = fn;                                                 \
		if ((temp_rc != RCL_RET_OK))                                            \
		{                                                                       \
			printf("Failed status on line %d: %d. Continuing.\n", __LINE__, \
			       (int)temp_rc);                                           \
		}                                                                       \
	}

#define EXECUTE_EVERY_N_MS(MS, X)                  \
	do                                         \
	{                                          \
		static volatile int64_t init = -1; \
		if (init == -1)                    \
		{                                  \
			init = uxr_millis();       \
		}                                  \
		if (uxr_millis() - init > MS)      \
		{                                  \
			X;                         \
			init = uxr_millis();       \
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

rcl_timer_t timer;
rcl_publisher_t imu_publisher;
geometry_msgs__msg__Point32 imu_data;

static const struct device *imu_sensor = DEVICE_DT_GET(ACCEL_NODE);
struct sensor_value accel_x, accel_y, accel_z;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */
static bool sensor_initialize(void);
static bool create_entities(void);
static bool destroy_entities(void);
static void publish_sensor_data_callback(rcl_timer_t *, int64_t);
static inline float out_ev(struct sensor_value *);
/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================
 */
int main(void)
{
	rmw_uros_set_custom_transport(MICRO_ROS_FRAMING_REQUIRED,
				      (void *)&default_params, zephyr_transport_open,
				      zephyr_transport_close, zephyr_transport_write,
				      zephyr_transport_read);

	// HW initialization
	if (!sensor_initialize())
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
			EXECUTE_EVERY_N_MS(500,
					   state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1))
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

static bool sensor_initialize(void)
{
	int ret;

	if (!device_is_ready(imu_sensor))
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
	RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options,
					       &allocator));

	// create node
	RCCHECK(rclc_node_init_default(&node, NODE_NAME, NODE_NAMESPACE, &support));

	// create timer
	RCCHECK(rclc_timer_init_default(&timer, &support,
					RCL_MS_TO_NS(PUBLISH_PERIOD_MS),
					publish_sensor_data_callback));

	// create publisher
	RCCHECK(rclc_publisher_init_best_effort(
	    &imu_publisher, &node,
	    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Point32), TOPIC_NAME));

	// create executor
	executor = rclc_executor_get_zero_initialized_executor();
	RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
	RCCHECK(rclc_executor_add_timer(&executor, &timer));

	return true;
}

static bool destroy_entities(void)
{
	rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
	(void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

	RCSOFTCHECK(rcl_publisher_fini(&imu_publisher, &node));
	RCSOFTCHECK(rcl_timer_fini(&timer))
	RCSOFTCHECK(rclc_executor_fini(&executor));
	RCSOFTCHECK(rcl_node_fini(&node));
	RCSOFTCHECK(rclc_support_fini(&support));
	RCSOFTCHECK(rcl_node_options_fini(&init_options));
}

static void publish_sensor_data_callback(rcl_timer_t *timer,
					 int64_t last_call_time)
{
	RCLC_UNUSED(last_call_time);
	int ret;

	// read IMU
	ret = sensor_sample_fetch_chan(imu_sensor, SENSOR_CHAN_ACCEL_XYZ);
	if (ret < 0)
	{
		return;
	}
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_X, &accel_x);
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_Y, &accel_y);
	sensor_channel_get(imu_sensor, SENSOR_CHAN_ACCEL_Z, &accel_z);

	// fill IMU message
	imu_data.x = out_ev(&accel_x);
	imu_data.y = out_ev(&accel_y);
	imu_data.z = out_ev(&accel_z);

	RCSOFTCHECK(rcl_publish(&imu_publisher, &imu_data, NULL));
}

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}