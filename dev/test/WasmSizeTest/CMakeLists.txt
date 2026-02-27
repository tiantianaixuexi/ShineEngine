cmake_minimum_required(VERSION 3.28)
project(WasmSizeTests LANGUAGES C CXX)

# =========================================================
# WASM Size Test - 通用测试框架
# 每个子目录是一个独立的 wasm test，自动使用 smallwasm 的编译配置
# 
# 使用方式：
#   1. 在 dev/test/WasmSizeTest/ 下创建新目录（如 MyTest/）
#   2. 放入 test.cpp
#   3. 完成！CMake 会自动发现并构建
# =========================================================

# 工具链路径（从命令行传入或环境变量）
set(LLVM_ROOT "" CACHE PATH "Path to LLVM root")
set(WASMOPT "" CACHE FILEPATH "Path to wasm-opt")

# 如果命令行没有传入，从环境变量读取
if(NOT LLVM_ROOT AND DEFINED ENV{LLVM_ROOT})
    set(LLVM_ROOT "$ENV{LLVM_ROOT}")
endif()
if(NOT WASMOPT AND DEFINED ENV{WASMOPT})
    set(WASMOPT "$ENV{WASMOPT}")
endif()

find_program(SMALLWASM_CLANG NAMES clang clang.exe
    HINTS "${LLVM_ROOT}/bin" "${LLVM_ROOT}"
    REQUIRED
)
find_program(SMALLWASM_WASM_LD NAMES wasm-ld wasm-ld.exe
    HINTS "${LLVM_ROOT}/bin" "${LLVM_ROOT}"
    REQUIRED
)

# 编译标志（复用 smallwasm 的配置）
set(SMALLWASM_CXX_FLAGS
    --target=wasm32
    -Oz
    -ffreestanding -nostdlib
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -fno-unwind-tables -fno-asynchronous-unwind-tables
    -fvisibility=hidden
    -ffunction-sections -fdata-sections
    -flto=thin
    -std=c++23
)

# =========================================================
# 公共文件配置
# 所有 test 都会自动包含这些文件
# =========================================================
set(COMMON_TEST_SOURCES "")  # 公共源文件（会被编译到每个 test）
set(COMMON_TEST_INCLUDES "") # 公共头文件目录

# 添加公共头文件目录（所有 test 都会自动包含）
list(APPEND COMMON_TEST_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/common")

# 示例：添加公共源文件（会被编译到每个 test 中）
# list(APPEND COMMON_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/common/test_utils.cpp")

# =========================================================
# 收集所有 test 文件（用于 VS 显示）
# =========================================================
set(ALL_TEST_SOURCES "${CMAKE_CURRENT_LIST_FILE}")  # 包含本文件

# =========================================================
# 添加单个 wasm size test
# =========================================================
function(add_wasm_size_test TEST_NAME)
    set(TEST_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_NAME})
    set(TEST_BUILD_DIR ${CMAKE_BINARY_DIR}/${TEST_NAME})
    set(TEST_DIST_DIR ${TEST_BUILD_DIR}/dist)
    
    file(MAKE_DIRECTORY ${TEST_DIST_DIR})
    
    # 源文件和输出文件（使用 TEST_NAME 作为文件名前缀）
    set(TEST_CPP ${TEST_SRC_DIR}/test.cpp)
    set(TEST_OBJ ${TEST_BUILD_DIR}/${TEST_NAME}.o)
    set(TEST_RAW ${TEST_BUILD_DIR}/${TEST_NAME}.raw.wasm)
    set(TEST_WASM ${TEST_DIST_DIR}/${TEST_NAME}.wasm)
    set(TEST_WAT ${TEST_DIST_DIR}/${TEST_NAME}.wat)
    
    # 收集源文件用于 VS 显示
    list(APPEND ALL_TEST_SOURCES ${TEST_CPP})
    set(ALL_TEST_SOURCES ${ALL_TEST_SOURCES} PARENT_SCOPE)
    
    # 准备编译参数
    set(COMPILE_FLAGS ${SMALLWASM_CXX_FLAGS})
    foreach(inc_dir ${COMMON_TEST_INCLUDES})
        list(APPEND COMPILE_FLAGS "-I${inc_dir}")
    endforeach()
    
    # 编译 test.o（包含公共源文件）
    set(ALL_SOURCE_FILES ${TEST_CPP} ${COMMON_TEST_SOURCES})
    add_custom_command(OUTPUT ${TEST_OBJ}
        COMMAND ${SMALLWASM_CLANG} ${COMPILE_FLAGS} -c ${TEST_CPP} -o ${TEST_OBJ}
        DEPENDS ${ALL_SOURCE_FILES}
        COMMENT "Building ${TEST_NAME}/test.o"
        VERBATIM
    )
    
    # 链接 wasm（导出 test 函数，防止被 gc-sections 干掉）
    add_custom_command(OUTPUT ${TEST_RAW}
        COMMAND ${SMALLWASM_WASM_LD} 
            --no-entry 
            --export-memory 
            --allow-undefined
            --export=malloc --export=free
            --export=test_${TEST_NAME}_ops
            --gc-sections --lto-O2
            --strip-all
            ${TEST_OBJ}
            -o ${TEST_RAW}
        DEPENDS ${TEST_OBJ}
        COMMENT "Linking ${TEST_NAME}/${TEST_NAME}.wasm"
        VERBATIM
    )
    
    # wasm-opt 优化（生成 test.wasm）
    if(WASMOPT)
        add_custom_command(OUTPUT ${TEST_WASM}
            COMMAND ${WASMOPT} -Oz --strip-dwarf --strip-producers --strip-names --zero-filled-memory --converge ${TEST_RAW} -o ${TEST_WASM}
            DEPENDS ${TEST_RAW}
            COMMENT "Optimizing ${TEST_NAME}/${TEST_NAME}.wasm"
            VERBATIM
        )
    else()
        add_custom_command(OUTPUT ${TEST_WASM}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEST_RAW} ${TEST_WASM}
            DEPENDS ${TEST_RAW}
            VERBATIM
        )
    endif()
    
    # 生成 .wat 文本格式（方便查看）
    find_program(WASM2WAT NAMES wasm2wat wasm2wat.exe
        HINTS "${LLVM_ROOT}/bin" "${LLVM_ROOT}"
    )
    if(WASM2WAT)
        add_custom_command(OUTPUT ${TEST_WAT}
            COMMAND ${WASM2WAT} ${TEST_WASM} -o ${TEST_WAT}
            DEPENDS ${TEST_WASM}
            COMMENT "Generating ${TEST_NAME}/${TEST_NAME}.wat"
            VERBATIM
        )
    endif()
    
    # 创建 target（依赖 wasm 和 wat）
    if(WASM2WAT)
        add_custom_target(${TEST_NAME} ALL DEPENDS ${TEST_WASM} ${TEST_WAT})
    else()
        add_custom_target(${TEST_NAME} ALL DEPENDS ${TEST_WASM})
    endif()
    
    # 添加到统一目标
    add_dependencies(WasmSizeTests ${TEST_NAME})
    
    message(STATUS "  Added wasm size test: ${TEST_NAME}")
endfunction()

# =========================================================
# 创建统一的目标（用于 Build.bat 调用）
# =========================================================
add_custom_target(WasmSizeTests)

# =========================================================
# 自动发现所有 test
# =========================================================
file(GLOB TEST_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

set(TEST_COUNT 0)
foreach(TEST_DIR ${TEST_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR})
        # 检查是否有 test.cpp
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_DIR}/test.cpp)
            add_wasm_size_test(${TEST_DIR})
            math(EXPR TEST_COUNT "${TEST_COUNT} + 1")
        endif()
    endif()
endforeach()

message(STATUS "Found ${TEST_COUNT} wasm size tests")

# =========================================================
# 为 VS 创建文件浏览目标（可选）
# =========================================================
if(ALL_TEST_SOURCES)
    add_custom_target(WasmSizeTest_Files SOURCES ${ALL_TEST_SOURCES})
    set_target_properties(WasmSizeTest_Files PROPERTIES FOLDER "WasmSizeTest")
endif()
