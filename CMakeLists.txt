cmake_minimum_required(VERSION 3.20)
project(TRIAL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件目录（你 include 下分了子目录）
add_library(TRIAL_INCLUDES INTERFACE)
target_include_directories(TRIAL_INCLUDES INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/algorithm
    ${PROJECT_SOURCE_DIR}/include/graph
    ${PROJECT_SOURCE_DIR}/include/metrics
    ${PROJECT_SOURCE_DIR}/include/operation
)

# 收集模块源文件
file(GLOB SRC_ALGO     CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/algorithm/*.cpp")
file(GLOB SRC_GRAPH    CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/graph/*.cpp")
file(GLOB SRC_METRICS  CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/metrics/*.cpp")
file(GLOB SRC_OP       CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/operation/*.cpp")

# Trial main 可切换：默认 SpaceTrial.cpp
set(TRIAL_MAIN "${PROJECT_SOURCE_DIR}/src/trials/SpaceTrial.cpp"
    CACHE FILEPATH "Path to trial main cpp")

add_executable(TRIAL
    ${SRC_ALGO}
    ${SRC_GRAPH}
    ${SRC_METRICS}
    ${SRC_OP}
    ${TRIAL_MAIN}
)

target_link_libraries(TRIAL PRIVATE TRIAL_INCLUDES)

# 输出到项目根目录（与你现有习惯一致）
set_target_properties(TRIAL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
)
