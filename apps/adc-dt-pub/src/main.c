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

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <stdio.h>

#include <rcl/rcl.h>
#include <rclc/executor.h>

#include <std_msgs/msg/int32.h>
#include <micro_ros_utilities/string_utilities.h>

#include <microros_transports.h>
#include <rmw_microros/rmw_microros.h>

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) ||                                   \
    !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/* ============================================================================
 * CONFIGURATION AND CONSTANTS
 * ============================================================================
 */

#define NODE_NAME "zephyr_adc_dt"
#define NODE_NAMESPACE ""
#define TOPIC_NAME_MAX_LEN 32
#define DOMAIN_ID 0

/* Timing Configuration */
#define PUBLISH_PERIOD_MS CONFIG_TOPIC_PUBLISH_PERIOD_MS

#define DT_SPEC_AND_COMMA(node_id, prop, idx)                                  \
  ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Error Checking Macros */
#define RCCHECK(fn)                                                            \
  {                                                                            \
    rcl_ret_t temp_rc = fn;                                                    \
    if ((temp_rc != RCL_RET_OK)) {                                             \
      printf("Failed status on line %d: %d. Aborting.\n", __LINE__,            \
             (int)temp_rc);                                                    \
      return 1;                                                                \
    }                                                                          \
  }
#define RCSOFTCHECK(fn)                                                        \
  {                                                                            \
    rcl_ret_t temp_rc = fn;                                                    \
    if ((temp_rc != RCL_RET_OK)) {                                             \
      printf("Failed status on line %d: %d. Continuing.\n", __LINE__,          \
             (int)temp_rc);                                                    \
    }                                                                          \
  }

#define EXECUTE_EVERY_N_MS(MS, X)                                              \
  do {                                                                         \
    static volatile int64_t init = -1;                                         \
    if (init == -1) {                                                          \
      init = uxr_millis();                                                     \
    }                                                                          \
    if (uxr_millis() - init > MS) {                                            \
      X;                                                                       \
      init = uxr_millis();                                                     \
    }                                                                          \
  } while (0)

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================
 */

enum states {
  WAITING_AGENT,
  AGENT_AVAILABLE,
  AGENT_CONNECTED,
  AGENT_DISCONNECTED
} state;

typedef struct {
  rcl_publisher_t publisher;
  std_msgs__msg__Int32 msg;
  char topic_name[TOPIC_NAME_MAX_LEN];
} pub_ctx_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================
 */

const struct adc_dt_spec adc_channels[] = {
    DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

#define CHANNEL_COUNT ARRAY_SIZE(adc_channels)

uint16_t buf;
struct adc_sequence sequence = {
    .buffer = &buf,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(buf),
};

int err;
uint32_t count = 0;

rclc_support_t support;
rcl_init_options_t init_options;
rcl_node_t node;
rclc_executor_t executor;
rcl_allocator_t allocator;

rcl_timer_t timer;
pub_ctx_t publishers[CHANNEL_COUNT];

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================
 */

static bool adc_initialize(void);
static bool create_entities(void);
static void destroy_entities(void);
static void publish_adc_data_callback(rcl_timer_t *, int64_t);

/* ============================================================================
 * MAIN APPLICATION
 * ============================================================================
 */

int main(void) {
  rmw_uros_set_custom_transport(MICRO_ROS_FRAMING_REQUIRED,
                                (void *)&default_params, zephyr_transport_open,
                                zephyr_transport_close, zephyr_transport_write,
                                zephyr_transport_read);

  // HW initialization
  if (!adc_initialize()) {
    return -EIO;
  }

  // agent discovery state machine
  state = WAITING_AGENT;
  while (1) {
    switch (state) {
    case WAITING_AGENT:
      EXECUTE_EVERY_N_MS(500,
                         state = (RMW_RET_OK == rmw_uros_ping_agent(100, 1))
                                     ? AGENT_AVAILABLE
                                     : WAITING_AGENT;);
      break;
    case AGENT_AVAILABLE:
      state = (true == create_entities()) ? AGENT_CONNECTED : WAITING_AGENT;
      if (state == WAITING_AGENT) {
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
}

static bool create_entities(void) {
  allocator = rcl_get_default_allocator();

  // create init options
  init_options = rcl_get_zero_initialized_init_options();
  RCCHECK(rcl_init_options_init(&init_options, allocator));
  RCCHECK(rcl_init_options_set_domain_id(&init_options, DOMAIN_ID));
  RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options,
                                         &allocator));

  // create node
  RCCHECK(rclc_node_init_default(&node, NODE_NAME, NODE_NAMESPACE, &support));

  // create per-channel publishers
  for (size_t i = 0; i < CHANNEL_COUNT; i++) {
    snprintf(publishers[i].topic_name, TOPIC_NAME_MAX_LEN, "/%s/ain%d/mV",
             CONFIG_TOPIC_PREFIX, adc_channels[i].channel_id);
    RCCHECK(rclc_publisher_init(
        &publishers[i].publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        publishers[i].topic_name, &rmw_qos_profile_sensor_data));
  }

  // create timer
  RCCHECK(rclc_timer_init_default(&timer, &support,
                                  RCL_MS_TO_NS(PUBLISH_PERIOD_MS),
                                  publish_adc_data_callback));

  // create executor
  executor = rclc_executor_get_zero_initialized_executor();
  RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
  RCCHECK(rclc_executor_add_timer(&executor, &timer));

  return true;
}

static void destroy_entities(void) {
  rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
  (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);

  for (size_t i = 0; i < CHANNEL_COUNT; i++) {
    RCSOFTCHECK(rcl_publisher_fini(&publishers[i].publisher, &node));
  }
  RCSOFTCHECK(rcl_timer_fini(&timer))
  RCSOFTCHECK(rclc_executor_fini(&executor));
  RCSOFTCHECK(rcl_node_fini(&node))
  RCSOFTCHECK(rclc_support_fini(&support));
  RCSOFTCHECK(rcl_init_options_fini(&init_options));
}

static bool adc_initialize(void) {

  /* Configure channels individually prior to sampling. */
  for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
    if (!adc_is_ready_dt(&adc_channels[i])) {
      printf("ADC controller device %s not ready\n", adc_channels[i].dev->name);
      return false;
    }

    err = adc_channel_setup_dt(&adc_channels[i]);
    if (err < 0) {
      printf("Could not setup channel #%d (%d) \n", i, err);
      return false;
    }
  }
  return true;
}

static void publish_adc_data_callback(rcl_timer_t *timer,
                                      int64_t last_call_time) {
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    int32_t val_mv;

    // Fetch samples from all channels
    for (size_t i = 0; i < CHANNEL_COUNT; i++) {
      (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

      err = adc_read_dt(&adc_channels[i], &sequence);
      if (err < 0) {
        printf("Could not read (%d)\n", err);
        continue;
      }

      /*
       * If using differential mode, the 16 bit value
       * in the ADC sample buffer should be a signed 2's
       * complement value.
       */
      if (adc_channels[i].channel_cfg.differential) {
        val_mv = (int32_t)((int16_t)buf);
      } else {
        val_mv = (int32_t)buf;
      }
      err = adc_raw_to_millivolts_dt(&adc_channels[i], &val_mv);

      if (err < 0) {
        printf("Value in mV is not available (%d)\n", err);
        continue;
      }
      publishers[i].msg.data = val_mv;
    }

    // Publish all channels
    for (size_t i = 0; i < CHANNEL_COUNT; i++) {
      RCSOFTCHECK(
          rcl_publish(&publishers[i].publisher, &publishers[i].msg, NULL));
    }
  }
}
