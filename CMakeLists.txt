cmake_minimum_required(VERSION 3.16)
project(Sanguosha VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# 主程序（暂时注释，等待ViewModel和View完成）
# add_executable(Sanguosha
#     src/main.cpp
# )

# target_link_libraries(Sanguosha
#     PRIVATE
#     SanguoshaModel
# )

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
