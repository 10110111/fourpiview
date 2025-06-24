cmake_minimum_required(VERSION 3.10.0)
project(FourPiView VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()

add_definitions(-DPROJECT_VERSION=\"${CMAKE_PROJECT_VERSION}\")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 6.4 REQUIRED Core OpenGL OpenGLWidgets)

include(CheckIncludeFileCXX)
check_include_file_cxx("glm/glm.hpp" HAVE_GLM)
if(NOT HAVE_GLM)
    unset(HAVE_GLM CACHE)
    message(FATAL_ERROR "GLM was not found")
endif()

add_executable(fourpiview
                main.cpp
                Canvas.cpp
                MainWin.cpp
              )
target_link_libraries(fourpiview Qt6::Core Qt6::OpenGL Qt6::OpenGLWidgets)
