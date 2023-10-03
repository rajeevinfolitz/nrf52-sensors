#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

set(COMMON_PATH "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/common")
include(${COMMON_PATH}/common.cmake)

GET_DEVICE_CONFIG_FILES(${BOARD} boards)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrfx_example)


# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/model_handler.c
	"C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common/saadc_examples_common.c")
target_include_directories(app PRIVATE include
							 ${COMMON_PATH} "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common")

# NORDIC SDK APP END
