cmake_minimum_required(VERSION 3.20.0)

# Handle custom boilerplate, if defined
if(DEFINED CUSTOM_BOILERPLATE)
    include(${CUSTOM_BOILERPLATE})
    return()
endif()

set(COMMON_PATH "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/common")
include(${COMMON_PATH}/common.cmake)

GET_DEVICE_CONFIG_FILES(${BOARD} boards)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nrfx_example)

target_sources(app PRIVATE src/main.c "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common/saadc_examples_common.c")
target_include_directories(app PRIVATE ${COMMON_PATH} "C:/ncs/v2.4.2/modules/hal/nordic/nrfx/samples/src/nrfx_saadc/common")
