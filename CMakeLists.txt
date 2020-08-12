cmake_minimum_required(VERSION 3.10)
project(gp VERSION 0.1)

###########################################################
################### Preliminary setup #####################
###########################################################

# Load standard directories
include(GNUInstallDirs)

# Build options.
option(BUILD_TESTS "Build tests" ON)
option(BUILD_DOCUMENTATION "Build documentation" OFF)
set(CMAKE_CXX_FLAGS "-Wno-deprecated-declarations")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_CURRENT_DIR_IN_INTERFACE ON)
include(GenerateExportHeader)

# Extra variables.
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DSYSTEM_OSX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  add_definitions(-DSYSTEM_LINUX)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

add_definitions(-DGP_DATA_DIR="${CMAKE_SOURCE_DIR}/data/")

# Add cmake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
message("Cmake module path: ${CMAKE_MODULE_PATH}")

# Add cmake utilities.
include(cmake/Utils.cmake)

# Add dependencies.
include(cmake/Dependencies.cmake)

# Check for C++11 features and enable.
gp_enable_cpp11()

# Set the build type. Default to Release mode.
if(NOT CMAKE_BUILD_TYPE)
  message("Defaulting to building targets in Release mode.")
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# Find and build third party libraries.
add_subdirectory(external)

find_package(Ceres)
message("Ceres found: ${ceres_FOUND}")
include_directories(${ceres_SOURCE_DIR})
message("ceres_SOURCE_DIR: ${ceres_LIBS_DIR}")
message("Ceres_INCLUDE_DIR: ${CERES_INCLUDE_DIR}")

###########################################################
##################### Create Library  #####################
###########################################################

# Add the source directories to the search path so cmake can find our headers.
message("CMAKE_SOURCE_DIR is ${CMAKE_SOURCE_DIR}")
include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect source files and build the gp  object.
file(GLOB_RECURSE gp_srcs ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_library(gp STATIC ${gp_srcs} ${CERES_INCLUDE_DIR} ${CERES_LIBRARIES})

target_link_libraries(gp ceres gflags)

#target_include_directories(gp PUBLIC 
#        ${CMAKE_SOURCE_DIR}/include/kernel
#	${CMAKE_SOURCE_DIR}/include/process
#	${CMAKE_SOURCE_DIR}/include/optimization
#	${CMAKE_SOURCE_DIR}/include/utils
#)

generate_export_header(gp)
#add_subdirectory(src/kernels)
#add_subdirectory(src/process)
#add_library(gplib SHARED src/kernels src/process)

# Add the source directories to the search path so cmake can find our headers.
#message("CMAKE_SOURCE_DIR is ${CMAKE_SOURCE_DIR}")
#include_directories(${CMAKE_SOURCE_DIR}/include)

# Collect source files and build the gp  object.
#file(GLOB_RECURSE gp_srcs ${CMAKE_SOURCE_DIR}/src/*.cpp)
#add_library(gplib SHARED ${gp_srcs})

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
export(PACKAGE gp)

# Installation instructions
#install(TARGETS gp DESTINATION lib)

install(
 TARGETS ${PROJECT_NAME} EXPORT gpTargets
 LIBRARY DESTINATION lib
 ARCHIVE DESTINATION lib
 RUNTIME DESTINATION bin
 INCLUDES DESTINATION include
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
    FILES_MATCHING REGEX ".hpp"
    PERMISSIONS
        OWNER_READ OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

#foreach ( file ${INCLUDE_FILES} )
#    get_filename_component( dir ${file} DIRECTORY )
#    install( FILES ${file} DESTINATION include/${dir} )
#endforeach()


#install(
#  FILES "${CMAKE_CURRENT_BINARY_DIR}/gp/gpConfig.cmake
#	"${CMAKE_CURRENT_BINARY_DIR}/gp/gpConfigVersion.cmake"
#  DESTINATION ${LIB_INSTALL_DIR}/gp/cmake
#)

############################################################
##################3 Generate package #######################
############################################################
#https://cmake.org/cmake/help/v3.9/manual/cmake-packages.7.html

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/gp/gpConfigVersion.cmake"
  VERSION 0.0.1
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT gpTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/gp/gpTargets.cmake"
  NAMESPACE gp::
)
configure_file(cmake/gpConfig.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/gp/gpConfig.cmake"
  COPYONLY
)
#
## Install generated package
set(ConfigPackageLocation lib/cmake/gp)
install(EXPORT gpTargets
  FILE
    gpTargets.cmake
  NAMESPACE
    gp::
  DESTINATION
    ${ConfigPackageLocation}
)
#
install(
  FILES cmake/gpConfig.cmake
        "${CMAKE_CURRENT_BINARY_DIR}/gp/gpConfigVersion.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)


# Find and build executables.
# Set up output directory for executables.
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/)
#add_subdirectory(exec)

# Find and build tests.
if (BUILD_TESTS)
  message("Build tests is enabled.")
  add_subdirectory(test)
endif (BUILD_TESTS)

# Find and build documentation.
#if (BUILD_DOCUMENTATION)
#  message("Build documentation is enabled.")
#  add_subdirectory(documentation)
#endif (BUILD_DOCUMENTATION)
