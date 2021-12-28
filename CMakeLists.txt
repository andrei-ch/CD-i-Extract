cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(cdix VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT BOOST_INCLUDE_DIR)
	set(BOOST_INCLUDE_DIR "/usr/local/include")
endif()

if(NOT BOOST_LIB_DIR)
	set(BOOST_LIB_DIR "/usr/local/lib")
endif()

include_directories(SYSTEM
		${PROJECT_BINARY_DIR}/include
		${PROJECT_SOURCE_DIR}/src
		${BOOST_INCLUDE_DIR}
		)

configure_file(src/version.h.in include/version.h)

add_subdirectory(src/cdi_lib)

add_executable(cdix
		src/actions.cpp
		src/actions.h
		src/dyuv.cpp
		src/dyuv.h
		src/helper.cpp
		src/helper.h
		src/main.cpp
		)

add_dependencies(cdix
		cdi_lib
		)

target_link_directories(cdix
		PUBLIC ${BOOST_LIB_DIR}
		)

target_link_libraries(cdix
		cdi_lib
		png
		boost_program_options
		)

install(TARGETS cdix)
