#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gpio_interface.c)

# Add midi header files
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../VS1053_interface)