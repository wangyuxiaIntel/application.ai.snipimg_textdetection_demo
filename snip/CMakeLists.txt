# Copyright (C) 2018-2023 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

file(GLOB_RECURSE HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*")

source_group("src" FILES ${SOURCES})
source_group("include" FILES ${HEADERS})

add_library(snip STATIC ${HEADERS} ${SOURCES})
target_include_directories(snip PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(snip PRIVATE opencv_core opencv_imgcodecs opencv_videoio)
