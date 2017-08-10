project(detector_plotter)
cmake_minimum_required(VERSION 3.3)

set( CMAKE_CXX_STANDARD 11)

aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})

SET (CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(HDF5 COMPONENTS CXX HL REQUIRED)

set( HDF5_INCLUDE_DIRECTORIES
    /usr/lib/x86_64-linux-gnu/hdf5/serial/include)

INCLUDE_DIRECTORIES( ${HDF5_INCLUDE_DIRECTORIES} ${PYTHON_INCLUDE_DIRECTORIES})

target_link_libraries(${PROJECT_NAME}
    ${HDF5_LIBRARIES}
    ${HDF5_CXX_LIBRARIES})
