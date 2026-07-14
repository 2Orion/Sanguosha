cmake_minimum_required(VERSION 3.16)
project(Sanguosha VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# MSVC: 强制使用 UTF-8 编译和输出，解决中文乱码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Qt包查找
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
    message(FATAL_ERROR "需要 Qt6 或 Qt5.15+（含 Widgets 模块）来构建此项目")
endif()

# ==================== Model 层 ====================

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
    src/Model/GameState.h
    src/Model/Card.h
    src/Model/Character.h
    src/Model/Player.h
    src/Model/CardManager.h
    src/Model/GameRule.h
)

add_library(SanguoshaModel STATIC
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
)

target_include_directories(SanguoshaModel
    PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Common
)

target_link_libraries(SanguoshaModel
    PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)

# ==================== ViewModel 层 ====================

set(VIEWMODEL_SOURCES
    src/ViewModel/ActionViewModel.cpp
    src/ViewModel/GameViewModel.cpp
)

set(VIEWMODEL_HEADERS
    src/ViewModel/ActionViewModel.h
    src/ViewModel/GameViewModel.h
)

# ==================== View 层 ====================

set(VIEW_SOURCES
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

# ==================== App 层（组合根） ====================

set(APP_SOURCES
    src/App/SGSApp.cpp
)

set(APP_HEADERS
    src/App/SGSApp.h
)

# ==================== Qt 版可执行程序 ====================

add_executable(SanguoshaQt
    src/main.cpp
    ${VIEW_SOURCES}
    ${VIEW_HEADERS}
    ${VIEWMODEL_SOURCES}
    ${VIEWMODEL_HEADERS}
    ${APP_SOURCES}
    ${APP_HEADERS}
)

target_include_directories(SanguoshaQt PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Common
    ${CMAKE_SOURCE_DIR}/src/Model
    ${CMAKE_SOURCE_DIR}/src/ViewModel
    ${CMAKE_SOURCE_DIR}/src/View
    ${CMAKE_SOURCE_DIR}/src/App
)

target_link_libraries(SanguoshaQt PRIVATE
    SanguoshaModel
    Qt${QT_VERSION_MAJOR}::Widgets
)

# ==================== Model 层冒烟测试 ====================

enable_testing()

add_executable(ModelSmokeTest
    tests/smoke_test.cpp
)

target_include_directories(ModelSmokeTest PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Common
    ${CMAKE_SOURCE_DIR}/src/Model
)

target_link_libraries(ModelSmokeTest PRIVATE
    SanguoshaModel
    Qt${QT_VERSION_MAJOR}::Core
)

add_test(NAME ModelSmokeTest COMMAND ModelSmokeTest)
