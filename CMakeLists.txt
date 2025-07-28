cmake_minimum_required(VERSION 3.12.0)
project(FourPiView VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=return-type -Wall -Wextra")
endif()

add_definitions(-DPROJECT_VERSION=\"${CMAKE_PROJECT_VERSION}\")

if(ANDROID)
    set(androidSearchLibs CorePrivate)
    set(androidLinkLibs Qt6::CorePrivate)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 6.4 REQUIRED Core OpenGL OpenGLWidgets Sensors ${androidSearchLibs})

include(CheckIncludeFileCXX)
check_include_file_cxx("Eigen/Dense" HAVE_EIGEN)
if(NOT HAVE_EIGEN)
    find_package(Eigen3 REQUIRED)
    include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})
endif()

include(CPM)
CPMFindPackage(NAME exiv2
    URL https://github.com/Exiv2/exiv2/archive/refs/tags/v0.28.5.tar.gz
    URL_HASH SHA256=e1671f744e379a87ba0c984617406fdf8c0ad0c594e5122f525b2fb7c28d394d
    EXCLUDE_FROM_ALL yes
    OPTIONS "EXIV2_ENABLE_XMP OFF"
            "EXIV2_ENABLE_LENSDATA OFF"
            "EXIV2_ENABLE_BMFF OFF"
            "EXIV2_ENABLE_BROTLI OFF"
            "EXIV2_ENABLE_VIDEO OFF"
            "EXIV2_ENABLE_INIH OFF"
            "EXIV2_BUILD_EXIV2_COMMAND OFF")
if(exiv2_ADDED)
    set(exiv2libs Exiv2::exiv2lib)
else()
    set(exiv2libs exiv2)
endif()

qt6_add_resources(RES_SOURCES resources.qrc)

qt_add_executable(fourpiview
                    main.cpp
                    Utils.cpp
                    Canvas.cpp
                    Gallery.cpp
                    MainWin.cpp
                    ImageFinder.cpp
                    ${RES_SOURCES}
                    android/AndroidManifest.xml
                    android/build.gradle
                    android/res/values/libs.xml
                    android/res/xml/qtprovider_paths.xml
                 )
target_link_libraries(fourpiview PRIVATE Qt6::OpenGLWidgets Qt6::Sensors ${androidLinkLibs} ${exiv2libs})
set_target_properties(fourpiview PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
