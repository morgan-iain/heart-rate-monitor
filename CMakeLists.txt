# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly

# CMakeLists.txt for MAX30102 ESP32-S3 Project

cmake_minimum_required(VERSION 3.16)

# Set the project name
set(PROJECT_NAME "heart_rate_monitor")

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(${PROJECT_NAME})

# Component registration is handled automatically by ESP-IDF
# All .c files in main/ directory will be compiled
# All .h files in main/ directory will be included

# Optional: Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)