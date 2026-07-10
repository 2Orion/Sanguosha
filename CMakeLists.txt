cmake_minimum_required(VERSION 3.16)
project(Sanguosha VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: 强制使用 UTF-8 编译和输出，解决中文乱码
if(MSVC)
    add_compile_options(/utf-8)
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
