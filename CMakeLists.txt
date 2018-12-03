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

# Add support for building in conda environment
if (DEFINED ENV{CONDA_PREFIX})
   set(CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
   link_directories($ENV{CONDA_PREFIX}/lib)
endif()

# Check cmake version
cmake_minimum_required(VERSION 3.5)
include(InstallRequiredSystemLibraries)

# Project name
project (DataWriter)

# C/C++
enable_language(CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wno-deprecated")

#####################################
# Link SPT3g Libraries
#####################################
set(SPT3G_DIR $ENV{SPT3G_SOFTWARE_PATH})
link_directories(${SPT3G_DIR}/build/spt3g)

#####################################
# Find Rogue & Support Libraries
#####################################
if (DEFINED ENV{ROGUE_DIR})
   set(Rogue_DIR $ENV{ROGUE_DIR}/lib)
else()
   set(Rogue_DIR ${CMAKE_PREFIX_PATH}/lib)
endif()
find_package(Rogue)

find_package(Boost COMPONENTS thread)

#####################################
# Find python3
#####################################
set(DO_PYTHON 0)

find_package(PythonInterp 3.6)
find_package(PythonLibs 3.6)

#####################################
# Setup build
#####################################

# Include files
include_directories(${ROGUE_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${SPT3G_DIR}/core/include/core)
include_directories(${SPT3G_DIR}/core/include)

# Create rogue python library
add_library(DataWriter SHARED DataWriter.cpp)

# Set output to TOP/lib, remove lib prefix
set_target_properties(DataWriter PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/python)
set_target_properties(DataWriter PROPERTIES PREFIX "")

# Link to rogue core
TARGET_LINK_LIBRARIES(DataWriter LINK_PUBLIC ${ROGUE_LIBRARIES})
TARGET_LINK_LIBRARIES(DataWriter LINK_PUBLIC ${SPT3G_DIR}/build/spt3g/core.so)
