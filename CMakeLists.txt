cmake_minimum_required(VERSION 2.8)

project(dysco)

# Casacore has a separate CMake file in this directory
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake)

SET(CASACORE_MAKE_REQUIRED_EXTERNALS_OPTIONAL TRUE)
find_package(Casacore REQUIRED COMPONENTS casa ms tables measures)

find_library(GSL_LIB NAMES gsl)
find_path(GSL_INCLUDE_DIR NAMES gsl/gsl_version.h)
find_library(GSL_CBLAS_LIB NAMES gslcblas)

find_package(Threads REQUIRED)

find_package(Boost COMPONENTS system REQUIRED)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-pedantic -Wno-long-long -Werror=vla" COMPILER_SUPPORTS_PEDANTIC)
if(COMPILER_SUPPORTS_PEDANTIC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-long-long -Werror=vla")
endif(COMPILER_SUPPORTS_PEDANTIC)

include_directories(${CASACORE_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${GSL_INCLUDE_DIR})

set(CMAKE_CXX_FLAGS "-O3 -Wall -DNDEBUG -march=native --std=c++11")

add_library(dyscostman-object OBJECT
	aftimeblockencoder.cpp
	dyscostman.cpp
	dyscodatacolumn.cpp
	dyscoweightcolumn.cpp
	stochasticencoder.cpp
	threadeddyscocolumn.cpp
	rftimeblockencoder.cpp
	rowtimeblockencoder.cpp)
set_property(TARGET dyscostman-object PROPERTY POSITION_INDEPENDENT_CODE 1) 

# Note: casapy fails if Casa is linked in the storage manager, so we have to trust that
# casapy's version of casacore is binary compatible with this storage manager's casacore.
add_library(dyscostman SHARED $<TARGET_OBJECTS:dyscostman-object>)
target_link_libraries(dyscostman ${GSL_LIB} ${GSL_CBLAS_LIB} ${CASACORE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(dscompress dscompress.cpp stopwatch.cpp)
target_link_libraries(dscompress dyscostman ${GSL_LIB} ${GSL_CBLAS_LIB} ${CASACORE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_executable(decompress decompress.cpp)
target_link_libraries(decompress dyscostman ${GSL_LIB} ${GSL_CBLAS_LIB} ${CASACORE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# add target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

find_package(Boost COMPONENTS system filesystem unit_test_framework)
if(Boost_FOUND)
  add_executable(runtests EXCLUDE_FROM_ALL
    $<TARGET_OBJECTS:dyscostman-object>
    tests/runtests.cpp 
    tests/encodeexample.cpp
    tests/testbytepacking.cpp
    tests/testdithering.cpp
    tests/testdyscostman.cpp
    tests/testtimeblockencoder.cpp
    )
  target_link_libraries(runtests ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${GSL_LIB} ${GSL_CBLAS_LIB} ${CASACORE_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  add_test(runtests runtests)
  add_custom_target(check COMMAND runtests -l unit_scope DEPENDS runtests)
else()
  message("Boost testing framework not found.")
endif()

install (TARGETS dyscostman DESTINATION lib) 
install (TARGETS dscompress DESTINATION bin)
