cmake_minimum_required(VERSION 3.19)
project(mmbag_extract C)

add_subdirectory(libmspack)

add_executable(mmbag_extract mmbag_extract.c)
target_link_libraries(mmbag_extract libmspack::mspack)