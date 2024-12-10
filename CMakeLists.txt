# ----------------------------------------------------------------------------
# Title      : ROGUE Example Module CMAKE
# ----------------------------------------------------------------------------
# File       : CMakeLists.txt
# Created    : 2018-02-27
# ----------------------------------------------------------------------------
# This file is part of the rogue_example software. It is subject to
# the license terms in the LICENSE.txt file found in the top-level directory
# of this distribution and at:
#    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html.
# No part of the rogue_example software, including this file, may be
# copied, modified, propagated, or distributed except according to the terms
# contained in the LICENSE.txt file.
# ----------------------------------------------------------------------------

# Check cmake version
cmake_minimum_required (VERSION 3.1)
include(InstallRequiredSystemLibraries)

# cmake policies -- best to keep these in sync with spt3g!
if(POLICY CMP0060) # Suppress cmake stripping full paths from libraries in some cases
  cmake_policy(SET CMP0060 NEW)
endif()
cmake_policy(SET CMP0012 NEW) # Allow use of true in boolean expressions
if(POLICY CMP0042) # Enable RPATH on OSX
   cmake_policy(SET CMP0042 NEW)
endif()

# Project name
project (SmurfStreamer)

# C/C++
enable_language(CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated -O3")

#####################################
# Link SPT3g Libraries
#####################################
set(SPT3G_DIR $ENV{SPT3G_SOFTWARE_PATH})
set(SO3G_DIR $ENV{SO3G_DIR})
link_directories(${SPT3G_DIR}/build/spt3g)

#####################################
# Find Rogue & Support Libraries
#####################################
# Find Rogue
if (DEFINED ENV{ROGUE_DIR})
    set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
    set(Rogue_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()

find_package(Rogue)

if (DEFINED ENV{SMURF_DIR})
    set(smurf_DIR $ENV{SMURF_DIR}/lib)
else()
    set(smurf_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()

find_package(smurf REQUIRED)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS thread)

# #####################################
# # Find python3
# #####################################
# set(DO_PYTHON 0)
#
find_package(PythonInterp 3.6 REQUIRED)
find_package(PythonLibs 3.6 REQUIRED)

find_package (Python3 COMPONENTS Interpreter Development)

#####################################
# Setup build
#####################################
include_directories(/usr/local/include/)

# Include files
include_directories(${ROGUE_INCLUDE_DIRS})
include_directories(${SMURF_INCLUDE_DIRS})

include_directories(${SPT3G_DIR}/core/include/core)
include_directories(${SPT3G_DIR}/core/include)
include_directories(${SPT3G_DIR}/core/src)
include_directories(${SO3G_DIR}/include)


include_directories(/usr/local/include/)
include_directories(${PROJECT_SOURCE_DIR}/include/)

# Create rogue python library
AUX_SOURCE_DIRECTORY(src SRC_FILES)
add_library(SmurfStreamer SHARED ${SRC_FILES})

# Set output to TOP/lib, remove lib prefix
set_target_properties(SmurfStreamer PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set_target_properties(SmurfStreamer PROPERTIES PREFIX "")
set_target_properties(SmurfStreamer PROPERTIES OUTPUT_NAME "sosmurfcore")

# Link to rogue core
TARGET_LINK_LIBRARIES(SmurfStreamer LINK_PUBLIC ${ROGUE_LIBRARIES} ${SMURF_LIBRARIES})
TARGET_LINK_LIBRARIES(SmurfStreamer LINK_PUBLIC ${SPT3G_DIR}/build/lib/libspt3g-core.so)
TARGET_LINK_LIBRARIES(SmurfStreamer LINK_PUBLIC ${SO3G_DIR}/build/so3g/libso3g.so)
