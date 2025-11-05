cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_FIND_ROOT_PATH ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET})
list(APPEND CMAKE_MODULE_PATH ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/stb)
list(APPEND CMAKE_MODULE_PATH ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/share/libsquish)

project(PAKExtractor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")

find_package(Stb REQUIRED)
find_package(GIF REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenJPEG CONFIG REQUIRED)

include_directories("${CMAKE_SOURCE_DIR}/src/")
file(GLOB_RECURSE SOURCES src/*.cpp )
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_directories(${PROJECT_NAME} PRIVATE ${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib)
target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB openjp2 GIF::GIF squish)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
