#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sd_card_interface.c)


# Add spi_interface files
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../spi_interface.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)