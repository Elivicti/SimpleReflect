cmake_minimum_required (VERSION 3.11)
project(SimpleReflect LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20 REQUIRED)

add_subdirectory(examples)

install(DIRECTORY "include/SimpleReflect" DESTINATION include)
