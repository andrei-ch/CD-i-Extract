
add_library(cdi_lib
		debug.cpp
		debug.h
		media.h
		parse.h
		sector.cpp
		sector.h
		structure.cpp
		structure.h
		util.h
		)

target_link_directories(cdi_lib
		PUBLIC ${BOOST_LIB_DIR}
		)

target_link_libraries(cdi_lib
		boost_filesystem
		)
