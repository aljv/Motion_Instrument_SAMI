#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VS1053_interface.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VS10xx_uc.h)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/midi.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/rtmidistart.plg)

# Add spi_interface files
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../spi_interface.c)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)