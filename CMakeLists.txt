# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(sgs)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

include_directories(
    src
    src/Core
    src/Model
    src/View
    src/ViewModel
)

# ==================== 公共源文件（不含 main） ====================

set (CORE_SRCS
    src/Core/CommonTypes.h
    src/Core/Event.h
    src/Core/RandomUtils.h
)

set (MODEL_SRCS
    src/Model/Card.cpp
    src/Model/Card.h
    src/Model/CardManager.cpp
    src/Model/CardManager.h
    src/Model/Character.cpp
    src/Model/Character.h
    src/Model/CommonTypes.h
    src/Model/Event.h
    src/Model/GameRule.cpp
    src/Model/GameRule.h
    src/Model/GameState.cpp
    src/Model/GameState.h
    src/Model/Player.cpp
    src/Model/Player.h
    src/Model/RandomUtils.h
)

set (VIEWMODEL_SRCS
    src/ViewModel/ActionViewModel.cpp
    src/ViewModel/ActionViewModel.h
    src/ViewModel/CardViewModel.cpp
    src/ViewModel/CardViewModel.h
    src/ViewModel/GameViewModel.cpp
    src/ViewModel/GameViewModel.h
    src/ViewModel/PlayerViewModel.cpp
    src/ViewModel/PlayerViewModel.h
)

set (VIEW_SRCS
    src/View/ActionPanelWidget.cpp
    src/View/ActionPanelWidget.h
    src/View/CardWidget.cpp
    src/View/CardWidget.h
    src/View/GameBoardWidget.cpp
    src/View/GameBoardWidget.h
    src/View/HandCardAreaWidget.cpp
    src/View/HandCardAreaWidget.h
    src/View/MainWindow.cpp
    src/View/MainWindow.h
    src/View/PlayerInfoWidget.cpp
    src/View/PlayerInfoWidget.h
)

# ==================== 控制台版（无 Qt Widgets，仅 Core 用于 EventListener） ====================

add_executable(sgs_console
    ${CORE_SRCS}
    ${MODEL_SRCS}
    ${VIEWMODEL_SRCS}
    src/main.cpp
)

target_link_libraries(sgs_console PRIVATE Qt6::Core)

# ==================== Qt GUI 版 ====================

add_executable(sgs_qt
    ${CORE_SRCS}
    ${MODEL_SRCS}
    ${VIEWMODEL_SRCS}
    ${VIEW_SRCS}
    src/View/main_qt.cpp
)

target_link_libraries(sgs_qt PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# ==================== Model 冒烟测试 ====================

add_executable(sgs_test
    ${CORE_SRCS}
    ${MODEL_SRCS}
    tests/smoke_test.cpp
)

target_link_libraries(sgs_test PRIVATE Qt6::Core)
