cmake_minimum_required (VERSION 3.11)


include_directories("${CMAKE_SOURCE_DIR}/include")

if (MSVC)
	# for msvc, this flag is required, otherwise macros won't expand correctly
	add_compile_options(/Zc:preprocessor)
endif()

add_executable(class_example class_example.cpp)
add_executable( enum_example  enum_example.cpp)
