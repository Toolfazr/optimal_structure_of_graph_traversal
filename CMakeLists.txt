cmake_minimum_required(VERSION 3.20)
project(TRIAL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 头文件目录（你 include 下分了子目录）
add_library(TRIAL_INCLUDES INTERFACE)
target_include_directories(TRIAL_INCLUDES INTERFACE
    ${PROJECT_SOURCE_DIR}/Include
    ${PROJECT_SOURCE_DIR}/Include/GraphAlgorithm
    ${PROJECT_SOURCE_DIR}/Include/GraphAlgorithm/RankSeeking
    ${PROJECT_SOURCE_DIR}/Include/GraphGeneration
    ${PROJECT_SOURCE_DIR}/Include/GraphStorageStructure
    ${PROJECT_SOURCE_DIR}/Include/Utility
)

# 收集模块源文件
file(GLOB SRC_ALGO     CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Src/GraphAlgorithm/*.cpp")
file(GLOB SRC_ALGO_RS    CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Src/GraphAlgorithm/RankSeeking/*.cpp")
file(GLOB SRC_GEN  CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Src/GraphGeneration/*.cpp")
file(GLOB SRC_GSS  CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Src/GraphStorageStructure/*.cpp")
file(GLOB SRC_UT       CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/Src/Utility/*.cpp")

# Trial main 可切换：默认 RandomGraphTrial.cpp
set(TRIAL_MAIN "${PROJECT_SOURCE_DIR}/Src/Trials/RandomGraphTrial.cpp"
    CACHE FILEPATH "Path to trial main cpp")

add_executable(TRIAL
    ${SRC_ALGO}
    ${SRC_ALGO_RS}
    ${SRC_GEN}
    ${SRC_GSS}
    ${SRC_UT}
    ${TRIAL_MAIN}
)

target_link_libraries(TRIAL PRIVATE TRIAL_INCLUDES)

# 输出到项目根目录（与你现有习惯一致）
set_target_properties(TRIAL PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}
)
