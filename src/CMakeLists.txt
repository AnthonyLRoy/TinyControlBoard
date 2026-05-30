# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp)

FILE(GLOB_RECURSE canonical_sources
	${CMAKE_SOURCE_DIR}/lib/app/*.cpp
	${CMAKE_SOURCE_DIR}/lib/hal/*.cpp
	${CMAKE_SOURCE_DIR}/lib/indicators/*.cpp
	${CMAKE_SOURCE_DIR}/lib/input/actions/*.cpp
	${CMAKE_SOURCE_DIR}/lib/power/*.cpp
	${CMAKE_SOURCE_DIR}/lib/protocol/*.cpp)

idf_component_register(
	SRCS
		${app_sources}
		${canonical_sources}
	INCLUDE_DIRS
		"."
		"../lib")
