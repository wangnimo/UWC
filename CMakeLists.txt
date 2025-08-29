cmake_minimum_required(VERSION 3.16)

project(UWC VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Qt组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network Charts Sql Concurrent PrintSupport Help )
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network Charts Sql Concurrent PrintSupport Help )

# 源文件
set(SOURCES
    main.cpp
    mainwindow.cpp
    modules/cmvcamera.cpp
    modules/device_management.cpp
    modules/calibration.cpp
    modules/report_generator.cpp
    modules/result_verification.cpp
    modules/data_acquisition.cpp
    # modules/data_management.cpp
    modules/settings.cpp
)

# 头文件
set(HEADERS
    mainwindow.h
    Drawer.h
    modules/cmvcamera.h
    modules/device_management.h
    modules/calibration.h
    modules/report_generator.h
    modules/result_verification.h
    modules/data_acquisition.h
    # modules/data_management.h
    modules/settings.h
)


# UI文件
set(UIS
    mainwindow.ui
    modules/calibration.ui
    modules/device_management.ui
    modules/report_generator.ui
    modules/result_verification.ui
    modules/data_acquisition.ui
    # modules/data_management.ui
    modules/settings.ui
)


# 资源文件
set(RESOURCES
    resources/icons.qrc
    resources/qss.qrc
)

# 海康SDK配置
set(HIKVISION_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hikvision)
include_directories(${HIKVISION_SDK_PATH}/include)
link_directories(${HIKVISION_SDK_PATH}/lib/win64)

# 检查海康SDK文件是否存在
if(NOT EXISTS ${HIKVISION_SDK_PATH}/include/MvCameraControl.h)
    message(FATAL_ERROR "Hikvision SDK not found. Please place SDK in 3rdparty/hikvision")
endif()

# 如果OpenCV不在标准路径，需要手动指定
set(OpenCV_DIR "D:/opencv/build")  # 替换为你的OpenCV安装路径
find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "OpenCV not found! Please check OpenCV_DIR.")
endif()


# 生成可执行文件
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(UWC
        MANUAL_FINALIZATION
        ${SOURCES} ${HEADERS} ${UIS} ${RESOURCES}
        Drawer.cpp

    )
else()
    add_executable(UWC
        ${SOURCES} ${HEADERS} ${UIS} ${RESOURCES}
    )
endif()

# 链接库
target_link_libraries(UWC PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Help
    ${OpenCV_LIBS}  # 链接OpenCV库
    MvCameraControl  # 海康SDK库
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.UWC)
endif()
set_target_properties(UWC PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS UWC
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(UWC)
endif()
