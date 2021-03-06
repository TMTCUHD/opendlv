# OpenDLV - Software for driverless vehicles realized with OpenDaVINCI
# Copyright (C) 2015 Chalmers Revere
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

cmake_minimum_required(VERSION 2.8)

project(opendlv-tools)

###########################################################################
# Set the search path for .cmake files.
SET (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake.Modules" ${CMAKE_MODULE_PATH})
SET (CMAKE_MODULE_PATH "${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
SET (CMAKE_MODULE_PATH "${OPENDAVINCI_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})

###########################################################################
# Global compile flags.
include(CompileFlags)

###########################################################################
# Find CxxTest.
include(CheckCxxTestEnvironment)

##########################################################################
# Find DL and CPPExperimental, for dynamic loading of libs
FIND_PACKAGE (DL)
#FIND_PACKAGE (CPPexperimental)

###########################################################################
# Find OpenDaVINCI.
find_package(OpenDaVINCI REQUIRED)

###########################################################################
# Find AutomotiveData.
SET(AUTOMOTIVEDATA_DIR "${OPENDAVINCI_DIR}")
FIND_PACKAGE (AutomotiveData REQUIRED)

###########################################################################
# Find OpenDLV.
set(OPENDLV_DIR "${OPENDAVINCI_DIR}")
find_package(OpenDLV REQUIRED)

###########################################################################
# Find OpenDLV data structures.
set(OPENDLVDATA_DIR "${CMAKE_INSTALL_PREFIX}")
find_package(ODVDOpenDLVData REQUIRED)

###########################################################################
# Find ODVDVehicle.
SET (CMAKE_MODULE_PATH "${ODVDVEHICLE_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
FIND_PACKAGE (ODVDVehicle REQUIRED)

###########################################################################
# Set common include directories.
INCLUDE_DIRECTORIES(SYSTEM ${AUTOMOTIVEDATA_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENDAVINCI_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENDLV_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(SYSTEM ${ODVDVEHICLE_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDOPENDLVDATA_INCLUDE_DIRS})
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})
include_directories(SYSTEM ${KALMAN_INCLUDE_DIR})
include_directories(SYSTEM ${TINYCNN_INCLUDE_DIR})
INCLUDE_DIRECTORIES(SYSTEM ${DL_INCLUDE_DIRS})

###########################################################################
# Set libraries to link against.
set(LIBRARIES ${OPENDAVINCI_LIBRARIES}
              ${OPENDLV_LIBRARIES}
              ${AUTOMOTIVEDATA_LIBRARIES}
              ${ODVDVEHICLE_LIBRARIES}
              ${ODVDOPENDLVDATA_LIBRARIES}
              ${DL_LIBRARIES} 
              ${CPPEXPERIMENTAL_LIBRARIES})

#add_subdirectory(vision)
add_subdirectory(signaladapter)
