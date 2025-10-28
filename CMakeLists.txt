cmake_minimum_required(VERSION 3.5)

project(coinbrowser LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Sql Network REQUIRED)
#find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)
find_package(OpenSSL REQUIRED)



set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        coinfilterdialog.cpp
        coinfilterdialog.h
        coinfilterdialog.ui
        settingsdialog.cpp
        settingsdialog.h
        settingsdialog.ui
        resources.qrc
        stocksdialog.cpp
        stocksdialog.h
        stocksdialog.ui
        worker.cpp
        worker.h
        settingsmanager.cpp
        settingsmanager.h
        jsonfilemanager.cpp
        jsonfilemanager.h
        constants.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(coinbrowser
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(coinbrowser SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(coinbrowser
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(coinbrowser PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
#target_link_libraries(coinbrowser PRIVATE Qt${QT_VERSION_MAJOR}::Gui)
target_link_libraries(coinbrowser PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(coinbrowser PRIVATE Qt${QT_VERSION_MAJOR}::Network)
#target_link_libraries(coinbrowser OpenSSL::SSL)
target_link_libraries(coinbrowser PRIVATE OpenSSL::applink)
