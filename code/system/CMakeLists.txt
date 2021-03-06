# OpenDLV - Software for driverless vehicles realized with OpenDaVINCI
# Copyright (C) 2015 Chalmers REVERE
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

project(opendlv-system)

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

###########################################################################
# Find OpenDaVINCI.
find_package(OpenDaVINCI REQUIRED)

###########################################################################
# Find AutomotiveData.
SET(AUTOMOTIVEDATA_DIR "${OPENDAVINCI_DIR}")
find_package (AutomotiveData REQUIRED)

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
find_package (ODVDVehicle REQUIRED)

###########################################################################
# Find ODVDV2V.
SET (CMAKE_MODULE_PATH "${ODVDVEHICLE_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
find_package (ODVDV2V REQUIRED)

###########################################################################
# Find ODVDVehicle.
SET (CMAKE_MODULE_PATH "${ODVDFH16TRUCK_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
find_package (ODVDFH16Truck REQUIRED)

###########################################################################
# Find ODVDTrimble.
SET (CMAKE_MODULE_PATH "${ODVDTRIMBLE_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
find_package (ODVDTrimble REQUIRED)

###########################################################################
# Find ODVDIMU.
SET (CMAKE_MODULE_PATH "${ODVDIMU_DIR}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/Modules" ${CMAKE_MODULE_PATH})
find_package (ODVDIMU REQUIRED)

###########################################################################
# Set common include directories.
include_directories(SYSTEM ${AUTOMOTIVEDATA_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENDAVINCI_INCLUDE_DIRS})
include_directories(SYSTEM ${OPENDLV_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDV2V_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDVEHICLE_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDFH16TRUCK_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDOPENDLVDATA_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDTRIMBLE_INCLUDE_DIRS})
include_directories(SYSTEM ${ODVDIMU_INCLUDE_DIRS})
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})
include_directories(SYSTEM ${KALMAN_INCLUDE_DIR})
include_directories(SYSTEM ${TINYCNN_INCLUDE_DIR})

###########################################################################
# Set libraries to link against.
set(LIBRARIES ${OPENDAVINCI_LIBRARIES}
              ${OPENDLV_LIBRARIES}
              ${AUTOMOTIVEDATA_LIBRARIES}
              ${ODVDVEHICLE_LIBRARIES}
              ${ODVDOPENDLVDATA_LIBRARIES}
              ${ODVDFH16TRUCK_LIBRARIES}
              ${ODVDOPENDLVDATA_LIBRARIES}
              ${ODVDTRIMBLE_LIBRARIES}
              ${ODVDIMU_LIBRARIES}
              ${ODVDV2V_LIBRARIES})

###########################################################################
# Add subfolders with sources.
#add_subdirectory(core)
add_subdirectory(application)

#install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../config/configuration DESTINATION . COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/system/application/perception/detectvehicle/trained_cnn DESTINATION ./share/opendlv/system/application/perception/detectvehicle/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/system/application/perception/detectvehicle/normalization_constants DESTINATION ./share/opendlv/system/application/perception/detectvehicle/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/leftCameraTransformationMatrix.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/rightCameraTransformationMatrix.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/leftCameraWarpPoints.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/rightCameraWarpPoints.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/leftCameraTransformationMatrixWarped.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/tools/vision/projection/rightCameraTransformationMatrixWarped.csv DESTINATION ./share/opendlv/tools/vision/projection/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/system/application/perception/detectlane/leftCameraRegions.csv DESTINATION ./share/opendlv/system/application/perception/detectlane/ COMPONENT system)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../resources/system/application/perception/detectlane/rightCameraRegions.csv DESTINATION ./share/opendlv/system/application/perception/detectlane/ COMPONENT system)

