cmake_minimum_required (VERSION 3.11)


add_executable(class_example class_example.cpp)
add_executable( enum_example  enum_example.cpp)

target_link_libraries(class_example SimpleReflect)
target_link_libraries( enum_example SimpleReflect)