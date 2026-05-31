cmake_minimum_required(VERSION 3.21.2)

project(DaMaSCUS-SUN-TrajectoryTXT VERSION "0.1.0")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/damascus/include")
  set(DEFAULT_DAMASCUS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/damascus")
else()
  get_filename_component(DEFAULT_DAMASCUS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
set(DAMASCUS_ROOT "${DEFAULT_DAMASCUS_ROOT}" CACHE PATH "DaMaSCUS-SUN source root or vendored source root")
get_filename_component(DAMASCUS_ROOT "${DAMASCUS_ROOT}" ABSOLUTE)

set(DAMASCUS_INCLUDE_DIR "${DAMASCUS_ROOT}/include")
set(DAMASCUS_SRC_DIR "${DAMASCUS_ROOT}/src")
set(DAMASCUS_DATA_DIR "${DAMASCUS_ROOT}/data")
set(OBSCURA_DIR "${DAMASCUS_ROOT}/external/obscura")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
option(ENABLE_EARTH_MODEL "Build Earth model support when Earth_Model files exist" ON)

if(NOT EXISTS "${DAMASCUS_INCLUDE_DIR}/Simulation_Trajectory.hpp")
  message(FATAL_ERROR "DAMASCUS_ROOT does not contain include/Simulation_Trajectory.hpp: ${DAMASCUS_ROOT}")
endif()
if(NOT EXISTS "${DAMASCUS_INCLUDE_DIR}/Celestial_Model.hpp")
  message(FATAL_ERROR "DAMASCUS_ROOT must point to the refactored DaMaSCUS source tree and contain include/Celestial_Model.hpp: ${DAMASCUS_ROOT}")
endif()
if(NOT EXISTS "${DAMASCUS_DATA_DIR}/model_agss09.dat")
  message(FATAL_ERROR "DAMASCUS_ROOT does not contain data/model_agss09.dat: ${DAMASCUS_ROOT}")
endif()
if(NOT EXISTS "${OBSCURA_DIR}/CMakeLists.txt")
  message(FATAL_ERROR "DAMASCUS_ROOT does not contain external/obscura: ${DAMASCUS_ROOT}")
endif()

set(DAMASCUS_HAS_EARTH_MODEL OFF)
if(ENABLE_EARTH_MODEL AND EXISTS "${DAMASCUS_INCLUDE_DIR}/Earth_Model.hpp" AND EXISTS "${DAMASCUS_SRC_DIR}/Earth_Model.cpp")
  if(NOT EXISTS "${DAMASCUS_DATA_DIR}/earth_prem.dat")
    message(FATAL_ERROR "Earth model support is enabled but DAMASCUS_ROOT does not contain data/earth_prem.dat: ${DAMASCUS_ROOT}")
  endif()
  set(DAMASCUS_HAS_EARTH_MODEL ON)
elseif(ENABLE_EARTH_MODEL)
  message(STATUS "Earth model support disabled: Earth_Model.hpp/cpp not found under DAMASCUS_ROOT=${DAMASCUS_ROOT}")
endif()

if(DAMASCUS_HAS_EARTH_MODEL)
  set(DAMASCUS_HAS_EARTH_MODEL_DEFINE 1)
else()
  set(DAMASCUS_HAS_EARTH_MODEL_DEFINE 0)
endif()

set(Boost_ADDITIONAL_VERSIONS "1.77.0" "1.77")
find_package(MPI REQUIRED)
find_package(Boost 1.77.0 EXACT REQUIRED)

if(NOT TARGET coverage_config)
  add_library(coverage_config INTERFACE)
endif()

set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
add_subdirectory("${OBSCURA_DIR}" "${CMAKE_BINARY_DIR}/obscura" EXCLUDE_FROM_ALL)

set(PROJECT_DIR "${DAMASCUS_ROOT}/")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in"
  "${GENERATED_DIR}/version.hpp")

set(DAMASCUS_TRAJECTORY_CORE_SOURCES
  "${DAMASCUS_SRC_DIR}/Dark_Photon.cpp"
  "${DAMASCUS_SRC_DIR}/Simulation_Trajectory.cpp"
  "${DAMASCUS_SRC_DIR}/Simulation_Utilities.cpp"
  "${DAMASCUS_SRC_DIR}/Solar_Model.cpp")

if(DAMASCUS_HAS_EARTH_MODEL)
  list(APPEND DAMASCUS_TRAJECTORY_CORE_SOURCES "${DAMASCUS_SRC_DIR}/Earth_Model.cpp")
endif()

add_library(damascus_trajectory_core STATIC
  ${DAMASCUS_TRAJECTORY_CORE_SOURCES})

target_compile_options(damascus_trajectory_core PUBLIC -Wall -pedantic)
target_compile_definitions(damascus_trajectory_core PUBLIC DAMASCUS_HAS_EARTH_MODEL=${DAMASCUS_HAS_EARTH_MODEL_DEFINE})
target_include_directories(damascus_trajectory_core
  PUBLIC
    "${DAMASCUS_INCLUDE_DIR}"
    "${GENERATED_DIR}"
    ${MPI_INCLUDE_PATH})
target_link_libraries(damascus_trajectory_core
  PUBLIC
    coverage_config
    libobscura
    ${MPI_CXX_LIBRARIES})

add_executable(DaMaSCUS-SUN-TrajectoryTXT
  src/main.cpp)

target_compile_options(DaMaSCUS-SUN-TrajectoryTXT PUBLIC -Wall -pedantic)
target_include_directories(DaMaSCUS-SUN-TrajectoryTXT
  PRIVATE
    "${DAMASCUS_INCLUDE_DIR}"
    "${GENERATED_DIR}"
    ${MPI_INCLUDE_PATH})
target_link_libraries(DaMaSCUS-SUN-TrajectoryTXT
  PRIVATE
    damascus_trajectory_core)

if(DAMASCUS_HAS_EARTH_MODEL)
  add_executable(DaMaSCUS-EarthModelCheck
    src/earth_model_check.cpp)

  target_compile_options(DaMaSCUS-EarthModelCheck PUBLIC -Wall -pedantic)
  target_include_directories(DaMaSCUS-EarthModelCheck
    PRIVATE
      "${DAMASCUS_INCLUDE_DIR}"
      "${GENERATED_DIR}"
      ${MPI_INCLUDE_PATH})
  target_link_libraries(DaMaSCUS-EarthModelCheck
    PRIVATE
      damascus_trajectory_core)
endif()

install(TARGETS DaMaSCUS-SUN-TrajectoryTXT DESTINATION bin)