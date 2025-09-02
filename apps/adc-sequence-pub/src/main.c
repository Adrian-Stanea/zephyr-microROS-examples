/*
 * Copyright (c) 2024 Centro de Inovacao EDGE
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

#include <micro_ros_utilities/string_utilities.h>
#include <std_msgs/msg/int32_multi_array.h>

#include <microros_transports.h>
#include <rmw_microros/rmw_microros.h>

/* ============================================================================
 * CONFIGURATION AND CONSTANTS
 * ============================================================================
 */

#define NODE_NAME "zephyr_adc_sequence"
#define NODE_NAMESPACE ""
#define TOPIC_NAME_MAX_LEN 32
#define DOMAIN_ID 0

/* ADC Configuration */
#define ADC_NODE DT_ALIAS(adc0)
#define CHANNEL_VREF(node_id) DT_PROP_OR(node_id, zephyr_vref_mv, 0)

/* Timing Configuration */
#define PUBLISH_PERIOD_MS CONFIG_TOPIC_PUBLISH_PERIOD_MS

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
  std_msgs__msg__Int32MultiArray msg;
  char topic_name[TOPIC_NAME_MAX_LEN];
} pub_ctx_t;

/* ============================================================================
 * GLOBAL VARIABLES
 * ============================================================================
 */

/* Data of ADC device specified in devicetree. */
const struct device *adc = DEVICE_DT_GET(ADC_NODE);

/* Data array of ADC channels for the specified ADC. */
const struct adc_channel_cfg channel_cfgs[] = {
    DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (, ))};

/* Data array of ADC channel voltage references. */
uint32_t vrefs_mv[] = {DT_FOREACH_CHILD_SEP(ADC_NODE, CHANNEL_VREF, (, ))};

#define CHANNEL_COUNT ARRAY_SIZE(channel_cfgs)

#ifdef CONFIG_SEQUENCE_32BITS_REGISTERS
uint32_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#else
uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
#endif

/* Options for the sequence sampling. */
const struct adc_sequence_options options = {
    .extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
    .interval_us = CONFIG_SEQUENCE_ADC_SAMPLE_INTERVAL_US,
};

/* Configure the sampling sequence to be made. */
struct adc_sequence sequence = {
    .buffer = channel_reading,
    /* buffer size in bytes, not number of samples */
    .buffer_size = sizeof(channel_reading),
    .resolution = CONFIG_SEQUENCE_RESOLUTION,
    .options = &options,
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

static void publish_adc_data_callback(rcl_timer_t *timer,
                                      int64_t last_call_time) {
  RCLC_UNUSED(last_call_time);
  if (timer != NULL) {
    err = adc_read(adc, &sequence);
    if (err < 0) {
      return;
    }
    int32_t val_mv;
    // Copy all samples for each channel to the corresponding publisher's
    // message as mV
    for (size_t chn_idx = 0; chn_idx < CHANNEL_COUNT; chn_idx++) {
      for (size_t sample_idx = 0; sample_idx < CONFIG_SEQUENCE_SAMPLES;
           sample_idx++) {
        val_mv = channel_reading[sample_idx][chn_idx];
        err =
            adc_raw_to_millivolts(vrefs_mv[chn_idx], channel_cfgs[chn_idx].gain,
                                  CONFIG_SEQUENCE_RESOLUTION, &val_mv);

        // Use RAW values if Conversion to mV is not supported
        if ((err < 0) || vrefs_mv[chn_idx] == 0) {
          publishers[chn_idx].msg.data.data[sample_idx] =
              channel_reading[sample_idx][chn_idx];
        } else {
          publishers[chn_idx].msg.data.data[sample_idx] = val_mv;
        }
      }
    }

    // Publish all channels
    for (size_t i = 0; i < CHANNEL_COUNT; i++) {
      RCSOFTCHECK(
          rcl_publish(&publishers[i].publisher, &publishers[i].msg, NULL));
    }
  }
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

  // allocate memory for interfaces
  for (size_t i = 0; i < CHANNEL_COUNT; i++) {
    publishers[i].msg.data.capacity = CONFIG_SEQUENCE_SAMPLES;
    publishers[i].msg.data.size = CONFIG_SEQUENCE_SAMPLES;
    publishers[i].msg.data.data =
        (int32_t)malloc(publishers[i].msg.data.capacity * sizeof(int32_t));
    memset(publishers[i].msg.data.data, 0,
           publishers[i].msg.data.capacity * sizeof(int32_t));

    publishers[i].msg.layout.dim.capacity = 1;
    publishers[i].msg.layout.dim.size = 1;
    publishers[i].msg.layout.dim.data =
        (std_msgs__msg__MultiArrayDimension *)malloc(
            publishers[i].msg.layout.dim.capacity *
            sizeof(std_msgs__msg__MultiArrayDimension));

    for (size_t j = 0; j < publishers[i].msg.layout.dim.capacity; j++) {
      publishers[i].msg.layout.dim.data[j].label =
          micro_ros_string_utilities_init("samples");
      publishers[i].msg.layout.dim.data[j].size = CONFIG_SEQUENCE_SAMPLES;
      publishers[i].msg.layout.dim.data[j].stride = CONFIG_SEQUENCE_SAMPLES;
    }
  }

  // create per-channel publishers
  for (size_t i = 0; i < CHANNEL_COUNT; i++) {
    snprintf(publishers[i].topic_name, TOPIC_NAME_MAX_LEN, "/%s/ain%d/mV",
             CONFIG_TOPIC_PREFIX, channel_cfgs[i].channel_id);
    RCCHECK(rclc_publisher_init(
        &publishers[i].publisher, &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32MultiArray),
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
    for (size_t j = 0; j < publishers[i].msg.layout.dim.capacity; j++) {
      micro_ros_string_utilities_destroy(
          &publishers[i].msg.layout.dim.data[j].label);
    }
    free(publishers[i].msg.layout.dim.data);
    free(publishers[i].msg.data.data);
    RCSOFTCHECK(rcl_publisher_fini(&publishers[i].publisher, &node));
  }
  RCSOFTCHECK(rcl_timer_fini(&timer))
  RCSOFTCHECK(rclc_executor_fini(&executor));
  RCSOFTCHECK(rcl_node_fini(&node))
  RCSOFTCHECK(rclc_support_fini(&support));
  RCSOFTCHECK(rcl_init_options_fini(&init_options));
}

static bool adc_initialize(void) {
  if (!device_is_ready(adc)) {
    return false;
  }

  /* Configure channels individually prior to sampling. */
  for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
    sequence.channels |= BIT(channel_cfgs[i].channel_id);
    err = adc_channel_setup(adc, &channel_cfgs[i]);
    if (err < 0) {
      printf("Could not setup channel #%d (%d)\n", i, err);
      return false;
    }
    if (channel_cfgs[i].reference == ADC_REF_INTERNAL) {
      vrefs_mv[i] = adc_ref_internal(adc);
    }
  }
  return true;
}
