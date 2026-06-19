# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Amoled143_Project)
# Dodaj tę linię na końcu głównego pliku CMakeLists.txt
add_compile_options("-Wno-error=dangling-pointer")