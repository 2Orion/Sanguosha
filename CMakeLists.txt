cmake_minimum_required(VERSION 3.16)
project(Sanguosha VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# MSVC: 强制使用 UTF-8 编译和输出，解决中文乱码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Qt包查找（可选：未安装Qt时跳过图形界面目标，仅构建Model/ViewModel/控制台版）
find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 5.15 QUIET COMPONENTS Widgets)
    if(Qt5_FOUND)
        set(QT_VERSION_MAJOR 5)
    endif()
endif()

if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(STATUS "未找到 Qt6/Qt5，将跳过图形界面版（SanguoshaQt）的构建")
endif()

# Model层源文件
set(MODEL_SOURCES
    src/Model/GameState.cpp
    src/Model/Card.cpp
    src/Model/Character.cpp
    src/Model/Player.cpp
    src/Model/CardManager.cpp
    src/Model/GameRule.cpp
)

set(MODEL_HEADERS
    src/Model/CommonTypes.h
    src/Model/Event.h
    src/Model/RandomUtils.h
    src/Model/GameState.h
    src/Model/Card.h
    src/Model/Character.h
    src/Model/Player.h
    src/Model/CardManager.h
    src/Model/GameRule.h
)

# 创建Model静态库
add_library(SanguoshaModel STATIC
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
)

target_include_directories(SanguoshaModel
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

# ViewModel层源文件
set(VIEWMODEL_SOURCES
    src/ViewModel/CardViewModel.cpp
    src/ViewModel/PlayerViewModel.cpp
    src/ViewModel/ActionViewModel.cpp
    src/ViewModel/GameViewModel.cpp
)

set(VIEWMODEL_HEADERS
    src/ViewModel/CardViewModel.h
    src/ViewModel/PlayerViewModel.h
    src/ViewModel/ActionViewModel.h
    src/ViewModel/GameViewModel.h
)

# View层源文件（Qt 图形界面版，仅在找到 Qt 时构建）
if(Qt6_FOUND OR Qt5_FOUND)
    set(VIEW_SOURCES
        src/View/main_qt.cpp
        src/View/CardWidget.cpp
        src/View/PlayerInfoWidget.cpp
        src/View/HandCardAreaWidget.cpp
        src/View/ActionPanelWidget.cpp
        src/View/GameBoardWidget.cpp
        src/View/MainWindow.cpp
    )

    set(VIEW_HEADERS
        src/View/CardWidget.h
        src/View/PlayerInfoWidget.h
        src/View/HandCardAreaWidget.h
        src/View/ActionPanelWidget.h
        src/View/GameBoardWidget.h
        src/View/MainWindow.h
    )

    # Qt 版可执行程序
    add_executable(SanguoshaQt
        ${VIEW_SOURCES}
        ${VIEW_HEADERS}
        ${VIEWMODEL_SOURCES}
        ${VIEWMODEL_HEADERS}
    )
    target_include_directories(SanguoshaQt PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/Model
        ${CMAKE_SOURCE_DIR}/src/ViewModel
    )
    target_link_libraries(SanguoshaQt PRIVATE
        SanguoshaModel
        Qt${QT_VERSION_MAJOR}::Widgets
    )
endif()

# 主程序（控制台版三国杀）
add_executable(Sanguosha
    src/main.cpp
    ${VIEWMODEL_SOURCES}
    ${VIEWMODEL_HEADERS}
)

target_include_directories(Sanguosha
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Model
)

target_link_libraries(Sanguosha
    PRIVATE
    SanguoshaModel
)

# Model层冒烟测试
enable_testing()

add_executable(ModelSmokeTest
    tests/smoke_test.cpp
)

target_include_directories(ModelSmokeTest
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/Model
)

target_link_libraries(ModelSmokeTest
    PRIVATE
    SanguoshaModel
)

add_test(NAME ModelSmokeTest COMMAND ModelSmokeTest)
