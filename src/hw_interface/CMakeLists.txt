#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/i2c_interface.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/spi_interface.c)
