#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(central_and_peripheral_hr)

# NORDIC SDK APP START
target_sources(app PRIVATE
  src/main.c
  src/ui_thread.c
)

# INCLUDE DIRECTORIES
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/fs)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/zephyr/subsys/fs)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/../modules/fs/fatfs)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hw_interface)

# APP SPECIFIC DIRECTORIES
add_subdirectory(src/hw_interface)
add_subdirectory(src/hw_interface/ble_interface)
add_subdirectory(src/hw_interface/VS1053_interface)
add_subdirectory(src/hw_interface/sd_card_interface)
add_subdirectory(src/hw_interface/inputs_interface)

# NORDIC SDK APP END
