# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# Declare a Zephyr library for this sensor
zephyr_library()

# Add max31723.c to zephyr lib sources
zephyr_library_sources_ifdef(CONFIG_MAX31723 max31723.c)

# Add conditional lib sources based on Kconfig
# NOTE: Omitted for now
# zephyr_library_sources_ifdef(CONFIG_MAX31723_TRIGGER max31723_trigger.c)

