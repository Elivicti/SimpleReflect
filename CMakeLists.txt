cmake_minimum_required (VERSION 3.11)
project(SimpleReflect LANGUAGES CXX VERSION 1.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
	${PROJECT_NAME} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

add_subdirectory(examples)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
	"${PROJECT_NAME}ConfigVersion.cmake"
	VERSION
		${PROJECT_VERSION}
	COMPATIBILITY
		SameMajorVersion
)
configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in"
	"${PROJECT_NAME}Config.cmake"
	INSTALL_DESTINATION
		"lib/cmake/${PROJECT_NAME}"
)

install(
	FILES
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
	DESTINATION
		lib/cmake/${PROJECT_NAME}
)

install(DIRECTORY "include/SimpleReflect" DESTINATION "include")
install(TARGETS "${PROJECT_NAME}" EXPORT "${PROJECT_NAME}Targets")
install(
	EXPORT "${PROJECT_NAME}Targets"
	FILE   "${PROJECT_NAME}Targets.cmake"
	DESTINATION
		lib/cmake/${PROJECT_NAME}
)