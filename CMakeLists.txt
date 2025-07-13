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

qt_add_executable(fourpiview
                    main.cpp
                    Canvas.cpp
                    MainWin.cpp
                    android/AndroidManifest.xml
                    android/build.gradle
                    android/res/values/libs.xml
                    android/res/xml/qtprovider_paths.xml
                 )
target_link_libraries(fourpiview PRIVATE Qt6::OpenGLWidgets)
set_target_properties(fourpiview PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
