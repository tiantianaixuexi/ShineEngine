cmake_minimum_required(VERSION 4.1.0)
project(ShineEngine VERSION 1.0.0 LANGUAGES CXX)

# --------------------------------
# Special configure modes
# --------------------------------
# 单独编译出 wasm 相关的 subcmake 模块（如 smallwasm），跳过其他模块
option(SHINE_WASM_DISPATCH_ONLY "Configure only wasm(subcmake) dispatch targets (quiet & fast)" OFF)
# When ON, suppress most module-related message(STATUS ...) output.
option(SHINE_QUIET_MODULE_MESSAGES "Suppress module status messages during configure" OFF)
# Target module name for selective output (empty = show all modules)
set(SHINE_TARGET_MODULE "" CACHE STRING "Target module name for selective output")
 
# C++20 模块支持
option(SHINE_BUILD_MODULE "Enable C++20 modules support" OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 构建类型配置
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Build configurations" FORCE)
else()
    set(allowedBuildTypes Debug Release)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowedBuildTypes}")
    if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowedBuildTypes)
        message(FATAL_ERROR "CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}' 无效，请使用 Debug 或 Release")
    endif()
endif()

option(REBUILD_DEPS "Rebuild dependent modules when building a single module" ON)

# Debug 统一追加后缀 d
set(CMAKE_DEBUG_POSTFIX d)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exe)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/exe)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# 平台特定配置
if(MSVC)
    add_compile_options(/utf-8 /MP /bigobj /Zc:__cplusplus)
    set(CMAKE_CXX_MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/modules)
    
    option(SHINE_ENABLE_AVX2 "Enable /arch:AVX2 for MSVC builds" OFF)
    if(SHINE_ENABLE_AVX2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
    endif()

    add_compile_options(
        $<$<CONFIG:Debug>:/Zi>
        $<$<CONFIG:Debug>:/FS>
        $<$<CONFIG:Release>:/FS>
    )

    add_link_options(
        $<$<CONFIG:Debug>:/DEBUG>
        $<$<CONFIG:Release>:/LTCG>
    )

    # 是否扫描 C++ 源文件以查找模块依赖关系
    if(SHINE_BUILD_MODULE)
        set(CMAKE_CXX_SCAN_FOR_MODULES ON)
    else()
        set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    endif()
endif()

# 全局编译定义
add_compile_definitions(
    SHINE_OPENGL
    STB_IMAGE_IMPLEMENTATION
    $<$<CONFIG:Release>:NDEBUG>
    $<$<BOOL:${SHINE_BUILD_MODULE}>:SHINE_USE_MODULE>
    $<$<BOOL:${SHINE_BUILD_MODULE}>:SHINE_MODULE_EXPORT=export>
    $<$<NOT:$<BOOL:${SHINE_BUILD_MODULE}>>:SHINE_MODULE_EXPORT=>
)

# 统一的包含目录
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/util
    ${CMAKE_SOURCE_DIR}/src/third
)

# 全局属性初始化
set_property(GLOBAL PROPERTY MODULE_DATA_CACHE "")

# ================================
# JSON解析辅助函数
# ================================
macro(parse_json_array json_content array_path result_var)
    set(${result_var} "")
    string(JSON length ERROR_VARIABLE error LENGTH ${json_content} ${array_path})
    if(NOT error AND length GREATER 0)
        math(EXPR length "${length} - 1")
        foreach(i RANGE 0 ${length})
            string(JSON item GET ${json_content} ${array_path} ${i})
            if(item)
                list(APPEND ${result_var} ${item})
            endif()
        endforeach()
    endif()
endmacro()

function(parse_json_type json_content result_var)
    set(type_list "")
    string(JSON type_type ERROR_VARIABLE type_error TYPE ${json_content} "type")
    if(NOT type_error)
        if(type_type STREQUAL "ARRAY")
            parse_json_array(${json_content} "type" type_list)
        elseif(type_type STREQUAL "STRING")
            string(JSON type_value GET ${json_content} "type")
            set(type_list ${type_value})
        endif()
    endif()
    if(NOT type_list)
        set(type_list "lib")
    endif()
    set(${result_var} ${type_list} PARENT_SCOPE)
endfunction()

# 解析 buildMode 字段：支持字符串 "editor"|"runtime"|"both" 或数组 ["editor", "runtime"]
function(parse_build_mode json_content result_var)
    set(build_mode_list "")
    
    # 检查 buildMode 字段
    string(JSON buildmode_type ERROR_VARIABLE buildmode_error TYPE ${json_content} "buildMode")
    if(NOT buildmode_error)
        if(buildmode_type STREQUAL "ARRAY")
            parse_json_array(${json_content} "buildMode" build_mode_list)
        elseif(buildmode_type STREQUAL "STRING")
            string(JSON buildmode_value GET ${json_content} "buildMode")
            set(build_mode_list ${buildmode_value})
        endif()
    endif()
    
    # 如果没有设置，默认为 "both"
    if(NOT build_mode_list)
        set(build_mode_list "both")
    endif()
    
    set(${result_var} ${build_mode_list} PARENT_SCOPE)
endfunction()

function(parse_third_party_libs json_content result_debug result_release)
    set(debug_list "")
    set(release_list "")
    string(JSON link_exists ERROR_VARIABLE link_error TYPE ${json_content} "link")
    if(NOT link_error)
        # CMake 的 string(JSON) 不支持点号分隔的嵌套路径，需要手动逐层解析
        # 先检查 link.debug 对象是否存在
        string(JSON debug_obj_exists ERROR_VARIABLE debug_obj_error TYPE ${json_content} "link" "debug")
        if(NOT debug_obj_error)
            # 获取 link.debug.lib 数组长度
            string(JSON debug_lib_length ERROR_VARIABLE debug_lib_error LENGTH ${json_content} "link" "debug" "lib")
            if(NOT debug_lib_error AND debug_lib_length GREATER 0)
                math(EXPR debug_lib_length "${debug_lib_length} - 1")
                foreach(i RANGE 0 ${debug_lib_length})
                    string(JSON lib_item GET ${json_content} "link" "debug" "lib" ${i})
                    if(lib_item)
                        list(APPEND debug_list ${lib_item})
                    endif()
                endforeach()
            endif()
        endif()
        
        # 先检查 link.release 对象是否存在
        string(JSON release_obj_exists ERROR_VARIABLE release_obj_error TYPE ${json_content} "link" "release")
        if(NOT release_obj_error)
            # 获取 link.release.lib 数组长度
            string(JSON release_lib_length ERROR_VARIABLE release_lib_error LENGTH ${json_content} "link" "release" "lib")
            if(NOT release_lib_error AND release_lib_length GREATER 0)
                math(EXPR release_lib_length "${release_lib_length} - 1")
                foreach(i RANGE 0 ${release_lib_length})
                    string(JSON lib_item GET ${json_content} "link" "release" "lib" ${i})
                    if(lib_item)
                        list(APPEND release_list ${lib_item})
                    endif()
                endforeach()
            endif()
        endif()
    endif()
    set(${result_debug} ${debug_list} PARENT_SCOPE)
    set(${result_release} ${release_list} PARENT_SCOPE)
endfunction()

# ================================
# 模块文件查找
# ================================
function(find_module_configs result_var base_dir)
    set(module_configs "")
    file(GLOB_RECURSE JSON_FILES "${base_dir}/*.json")
    foreach(json_file ${JSON_FILES})
        get_filename_component(file_name ${json_file} NAME)
        if(NOT file_name MATCHES "^\\.")
            list(APPEND module_configs ${json_file})
        endif()
    endforeach()
    set(${result_var} ${module_configs} PARENT_SCOPE)
endfunction()

function(find_module_file module_name result_var)
    get_property(ALL_MODULE_FILES GLOBAL PROPERTY "ALL_MODULE_FILES")
    foreach(file ${ALL_MODULE_FILES})
        get_filename_component(file_name ${file} NAME_WE)
        if(file_name STREQUAL ${module_name})
            set(${result_var} ${file} PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${result_var} "${CMAKE_SOURCE_DIR}/Module/${module_name}.json" PARENT_SCOPE)
endfunction()

# ================================
# 模块分类
# ================================
function(categorize_modules_by_type)
    set(modules ${ARGN})
    foreach(module_file ${modules})
        get_filename_component(module_name ${module_file} NAME_WE)
        file(READ ${module_file} json_content)
        parse_json_type(${json_content} module_types)
        foreach(type ${module_types})
            set(type_modules ${${type}_MODULES})
            list(APPEND type_modules ${module_name})
            set(${type}_MODULES ${type_modules} PARENT_SCOPE)
        endforeach()
    endforeach()
endfunction()

function(parse_and_categorize_modules)
    find_module_configs(MODULE_FILES "Module")
    find_module_configs(PROGRAM_FILES "Program")
    set(ALL_MODULE_FILES ${MODULE_FILES} ${PROGRAM_FILES})
    categorize_modules_by_type(${ALL_MODULE_FILES})
    # NOTE: "subcmake" is a nested/foreign CMake project built via a custom target (UTILITY).
    set(MODULE_TYPES lib static shared exe third interface subcmake)
    foreach(type ${MODULE_TYPES})
        set_property(GLOBAL PROPERTY "MODULES_BY_TYPE_${type}" "${${type}_MODULES}")
    endforeach()
    set_property(GLOBAL PROPERTY "ALL_MODULE_FILES" "${ALL_MODULE_FILES}")
endfunction()

# ================================
# 模块解析和缓存
# ================================
function(parse_and_cache_modules)
    parse_and_categorize_modules()
    get_property(ALL_MODULE_FILES GLOBAL PROPERTY "ALL_MODULE_FILES")

    foreach(module_file ${ALL_MODULE_FILES})
        get_filename_component(module_name ${module_file} NAME_WE)
        file(READ ${module_file} json_content)

        # 解析基本信息
        string(JSON name ERROR_VARIABLE name_error GET ${json_content} "name")
        if(name_error)
            set(name ${module_name})
        endif()

        # 解析数组字段
        parse_json_array(${json_content} "files" files_list)
        parse_json_array(${json_content} "files_module" files_module_list)
        parse_json_array(${json_content} "files_header" files_header_list)
        parse_json_array(${json_content} "deps" deps_list)
        parse_json_array(${json_content} "defines" defines_list)
        parse_json_array(${json_content} "dirs" dirs_list)
        parse_json_array(${json_content} "platform" platform_list)

        # 处理条件文件选择：根据 SHINE_BUILD_MODULE 选择 files_module 或 files_header
        if(files_module_list OR files_header_list)
            if(SHINE_BUILD_MODULE AND files_module_list)
                list(APPEND files_list ${files_module_list})
            elseif(NOT SHINE_BUILD_MODULE AND files_header_list)
                list(APPEND files_list ${files_header_list})
            endif()
        endif()

        # 处理平台特定文件
        if(WIN32)
            parse_json_array(${json_content} "files_windows" platform_files_list)
            list(APPEND files_list ${platform_files_list})

        elseif(ANDROID)
            parse_json_array(${json_content} "files_android" platform_files_list)
            list(APPEND files_list ${platform_files_list})
        endif()

        # 处理dirs字段，自动添加文件
        if(dirs_list)
            foreach(dir ${dirs_list})
                if(dir)
                    file(GLOB_RECURSE dir_found_files
                        "${CMAKE_SOURCE_DIR}/${dir}/*.h"
                        "${CMAKE_SOURCE_DIR}/${dir}/*.hpp"
                        "${CMAKE_SOURCE_DIR}/${dir}/*.ixx"
                        "${CMAKE_SOURCE_DIR}/${dir}/*.cpp"
                        "${CMAKE_SOURCE_DIR}/${dir}/*.cppm"
                    )
                    foreach(abs_path ${dir_found_files})
                        file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}" "${abs_path}")
                        list(APPEND files_list ${rel_path})
                    endforeach()
                endif()
            endforeach()
        endif()

        # 解析第三方库链接配置
        parse_third_party_libs(${json_content} third_lib_debug_list third_lib_release_list)

        # 解析类型
        parse_json_type(${json_content} type_list)

        # 解析 buildMode 配置（Editor/Runtime 区分）
        parse_build_mode(${json_content} build_mode_list)

        # 解析 subcmake 配置（可选）
        set(subcmake_source "")
        set(subcmake_build "")
        set(subcmake_generator "")
        set(subcmake_target "")
        set(subcmake_configure_args "")
        string(JSON subcmake_type ERROR_VARIABLE subcmake_err TYPE ${json_content} "subcmake")
        if(NOT subcmake_err)
            string(JSON subcmake_source ERROR_VARIABLE _e1 GET ${json_content} "subcmake" "source")
            string(JSON subcmake_build ERROR_VARIABLE _e2 GET ${json_content} "subcmake" "build")
            string(JSON subcmake_generator ERROR_VARIABLE _e3 GET ${json_content} "subcmake" "generator")
            string(JSON subcmake_target ERROR_VARIABLE _e4 GET ${json_content} "subcmake" "target")
            # NOTE: string(JSON) does NOT support dot-path. Must parse nested arrays via path segments.
            string(JSON cfg_len ERROR_VARIABLE cfg_err LENGTH ${json_content} "subcmake" "configure")
            if(NOT cfg_err AND cfg_len GREATER 0)
                math(EXPR cfg_len "${cfg_len} - 1")
                foreach(i RANGE 0 ${cfg_len})
                    string(JSON cfg_item GET ${json_content} "subcmake" "configure" ${i})
                    if(cfg_item)
                        list(APPEND subcmake_configure_args ${cfg_item})
                    endif()
                endforeach()
            endif()
        endif()

        # 缓存到全局属性
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_NAME" ${name})
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_FILES" "${files_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_DEPS" "${deps_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_DEFINES" "${defines_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG" "${third_lib_debug_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE" "${third_lib_release_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_PLATFORMS" "${platform_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_TYPES" "${type_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_BUILD_MODE" "${build_mode_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_SOURCE" "${subcmake_source}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_BUILD" "${subcmake_build}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_GENERATOR" "${subcmake_generator}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_TARGET" "${subcmake_target}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_CONFIGURE_ARGS" "${subcmake_configure_args}")
        
        # 预判断是否是 INTERFACE 库（用于后续依赖链接）
        set(is_interface_predicted FALSE)
        if("${type_list}" MATCHES "third" OR "${type_list}" MATCHES "interface")
            set(is_interface_predicted TRUE)
        else()
            # 检查是否是 header-only 库（只有头文件，没有源文件）
            set(has_source FALSE)
            set(has_header FALSE)
            foreach(file ${files_list})
                if(file MATCHES "\\.(cpp|ixx|cppm)$")
                    set(has_source TRUE)
                elseif(file MATCHES "\\.(h|hpp|ixx)$")
                    set(has_header TRUE)
                endif()
            endforeach()
            if(NOT has_source AND has_header)
                set(is_interface_predicted TRUE)
            endif()
        endif()
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_IS_INTERFACE" ${is_interface_predicted})

        # 添加到模块列表
        get_property(module_list GLOBAL PROPERTY MODULE_DATA_CACHE)
        list(APPEND module_list ${module_name})
        set_property(GLOBAL PROPERTY MODULE_DATA_CACHE "${module_list}")
    endforeach()
endfunction()

# ================================
# 依赖关系计算（缓存优化版本）
# ================================
function(compute_module_dependencies target_module result_var)
    set(dependencies "")
    set(visited_modules "")

    function(add_module_deps module_name visited_ref deps_ref)
        set(visited ${${visited_ref}})
        set(deps ${${deps_ref}})

        list(FIND visited ${module_name} found)
        if(NOT found EQUAL -1)
            return()
        endif()

        list(APPEND visited ${module_name})
        set(${visited_ref} ${visited} PARENT_SCOPE)

        get_property(module_deps GLOBAL PROPERTY "MODULE_${module_name}_DEPS")

        foreach(dep ${module_deps})
            if(dep)
                get_property(dep_name GLOBAL PROPERTY "MODULE_${dep}_NAME")
                if(dep_name)
                    list(FIND deps ${dep} dep_found)
                    if(dep_found EQUAL -1)
                        list(APPEND deps ${dep})
                    endif()

                    add_module_deps(${dep} visited_ref deps_ref)
                    set(deps ${${deps_ref}})
                endif()
            endif()
        endforeach()

        set(${deps_ref} ${deps} PARENT_SCOPE)
    endfunction()

    add_module_deps(${target_module} visited_modules dependencies)
    set(${result_var} ${dependencies} PARENT_SCOPE)
endfunction()

# 全局缓存变量
set(SHINE_TARGET_DEPS_CACHE "")

function(init_dependency_cache)
    if(NOT SHINE_TARGET_MODULE STREQUAL "" AND SHINE_TARGET_DEPS_CACHE STREQUAL "")
        compute_module_dependencies(${SHINE_TARGET_MODULE} deps)
        set(SHINE_TARGET_DEPS_CACHE "${deps}" CACHE INTERNAL "Target module dependencies" FORCE)
    endif()
endfunction()

function(should_show_module module_name)
    if(SHINE_TARGET_MODULE STREQUAL "")
        set(SHOULD_SHOW TRUE PARENT_SCOPE)
        return()
    endif()

    if(module_name STREQUAL SHINE_TARGET_MODULE)
        set(SHOULD_SHOW TRUE PARENT_SCOPE)
        return()
    endif()

    list(FIND SHINE_TARGET_DEPS_CACHE ${module_name} found)
    if(NOT found EQUAL -1)
        set(SHOULD_SHOW TRUE PARENT_SCOPE)
        return()
    endif()

    set(SHOULD_SHOW FALSE PARENT_SCOPE)
endfunction()

# 检查模块是否应该在当前构建模式下构建
# SHINE_BUILD_EDITOR=ON 表示 Editor 模式，OFF 表示 Runtime 模式
function(should_build_module module_name result_var)
    get_property(build_mode_list GLOBAL PROPERTY "MODULE_${module_name}_BUILD_MODE")
    
    # 如果没有设置 buildMode，默认为 "both"（总是构建）
    if(NOT build_mode_list)
        set(build_mode_list "both")
    endif()
    
    # 判断当前构建模式
    set(is_editor_mode FALSE)
    if(SHINE_BUILD_EDITOR)
        set(is_editor_mode TRUE)
    endif()
    
    # 检查 buildMode 列表
    set(should_build FALSE)
    
    # 如果 buildMode 是字符串（单个值）
    if(build_mode_list MATCHES "^both$")
        set(should_build TRUE)
    elseif(build_mode_list MATCHES "^editor$")
        if(is_editor_mode)
            set(should_build TRUE)
        endif()
    elseif(build_mode_list MATCHES "^runtime$")
        if(NOT is_editor_mode)
            set(should_build TRUE)
        endif()
    else()
        # buildMode 是数组，检查是否包含当前模式
        if(is_editor_mode)
            list(FIND build_mode_list "editor" found)
            if(NOT found EQUAL -1)
                set(should_build TRUE)
            endif()
        else()
            list(FIND build_mode_list "runtime" found)
            if(NOT found EQUAL -1)
                set(should_build TRUE)
            endif()
        endif()
        # 如果数组包含 "both"，总是构建
        list(FIND build_mode_list "both" found)
        if(NOT found EQUAL -1)
            set(should_build TRUE)
        endif()
    endif()
    
    set(${result_var} ${should_build} PARENT_SCOPE)
endfunction()

# ================================
# VS文件夹路径生成
# ================================
function(get_vs_folder_path module_file result_var)
    file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}/Module" ${module_file})
    get_filename_component(dir_path ${rel_path} DIRECTORY)
    if(dir_path AND NOT dir_path STREQUAL ".")
        set(folder_path "Modules/${dir_path}")
    else()
        set(folder_path "Modules")
    endif()
    set(${result_var} ${folder_path} PARENT_SCOPE)
endfunction()

# ================================
# 目标属性设置
# ================================
function(set_module_properties target_name)
    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe/"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/exe/"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/exe/"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe/"
        LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/exe/"
        LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/exe/"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/exe/"
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/exe/"
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/exe/"
    )
    
    # 在非模块模式下，禁用目标级别的模块扫描
    if(NOT SHINE_BUILD_MODULE AND MSVC)
        set_target_properties(${target_name} PROPERTIES
            CXX_MODULE OFF
            CXX_SCAN_FOR_MODULES OFF
        )
    endif()
endfunction()

# ================================
# 模块类型检查辅助函数
# ================================
function(check_module_type types type_to_check result_var)
    list(FIND types ${type_to_check} found)
    if(found EQUAL -1)
        set(${result_var} FALSE PARENT_SCOPE)
    else()
        set(${result_var} TRUE PARENT_SCOPE)
    endif()
endfunction()

# ================================
# 统一设置编译定义
# ================================
function(set_target_defines target_name defines platforms types is_interface is_third)
    if(is_interface)
        set(scope INTERFACE)
    elseif(is_third)
        set(scope PUBLIC)
    else()
        set(scope PRIVATE)
    endif()

    if(defines)
        target_compile_definitions(${target_name} ${scope} ${defines})
    endif()

    foreach(platform ${platforms})
        if(platform)
            string(TOUPPER ${platform} platform_upper)
            target_compile_definitions(${target_name} ${scope} "PLATFORM_${platform_upper}=1")
        endif()
    endforeach()

    foreach(type ${types})
        if(type)
            string(TOUPPER ${type} type_upper)
            target_compile_definitions(${target_name} ${scope} "BUILD_${type_upper}=1")
        endif()
    endforeach()

    # 根据 SHINE_BUILD_EDITOR 自动添加 BUILD_EDITOR 宏
    # Editor 模式：定义 BUILD_EDITOR
    # Runtime 模式：不定义 BUILD_EDITOR（或可选择性定义 BUILD_RUNTIME）
    if(SHINE_BUILD_EDITOR)
        target_compile_definitions(${target_name} ${scope} BUILD_EDITOR)
    endif()
endfunction()

# ================================
# 统一链接第三方库
# ================================
function(link_third_party_libs target_name third_libs_debug third_libs_release is_interface is_third)
    if(is_interface)
        set(scope INTERFACE)
    elseif(is_third)
        set(scope PUBLIC)
    else()
        set(scope PRIVATE)
    endif()

    if(third_libs_debug)
        foreach(lib ${third_libs_debug})
            target_link_libraries(${target_name} ${scope} 
                $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
        endforeach()
    endif()

    if(third_libs_release)
        foreach(lib ${third_libs_release})
            target_link_libraries(${target_name} ${scope} 
                $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
        endforeach()
    endif()
endfunction()

# ================================
# 解析并缓存所有模块数据
# ================================
parse_and_cache_modules()
get_property(MODULE_LIST GLOBAL PROPERTY MODULE_DATA_CACHE)

# 初始化依赖关系缓存
init_dependency_cache()

# ================================
# 创建所有模块目标
# ================================
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
    if(files)
        list(FILTER files EXCLUDE REGEX "CMakeLists.txt")
    endif()
    get_property(defines GLOBAL PROPERTY "MODULE_${module_name}_DEFINES")
    get_property(third_libs_debug GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG")
    get_property(third_libs_release GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE")
    get_property(platforms GLOBAL PROPERTY "MODULE_${module_name}_PLATFORMS")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")
    get_property(subcmake_source GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_SOURCE")
    get_property(subcmake_build GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_BUILD")
    get_property(subcmake_generator GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_GENERATOR")
    get_property(subcmake_target GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_TARGET")
    get_property(subcmake_configure_args GLOBAL PROPERTY "MODULE_${module_name}_SUBCMAKE_CONFIGURE_ARGS")

    find_module_file(${module_name} module_file)

    # subcmake 模块允许 files 为空（因为它不参与本工程编译，只负责触发子工程构建）
    check_module_type("${types}" "subcmake" is_subcmake)


    if(SHINE_WASM_DISPATCH_ONLY AND NOT is_subcmake)
        continue()
    endif()

    # 检查 buildMode：根据 Editor/Runtime 模式过滤模块
    should_build_module(${module_name} should_build)
    if(NOT should_build)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            get_property(build_mode_list GLOBAL PROPERTY "MODULE_${module_name}_BUILD_MODE")
            set(current_mode "Runtime")
            if(SHINE_BUILD_EDITOR)
                set(current_mode "Editor")
            endif()
            message(STATUS "Skip module (buildMode mismatch): ${name} (buildMode=${build_mode_list}, current=${current_mode})")
        endif()
        continue()
    endif()

    if(NOT files AND NOT is_subcmake)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "Skip empty module: ${name}")
        endif()
        continue()
    endif()

    # 分类文件类型
    set(header_files "")
    set(source_files "")
    foreach(file ${files})
        if(file MATCHES "\\.(h|hpp)$")
            list(APPEND header_files ${file})
        elseif(file MATCHES "\\.ixx$")
            # 在非模块模式下，.ixx 文件作为普通头文件
            if(NOT SHINE_BUILD_MODULE)
                list(APPEND header_files ${file})
            else()
                list(APPEND source_files ${file})
            endif()
        else()
            list(APPEND source_files ${file})
        endif()
    endforeach()

    # subcmake：独立子工程构建（UTILITY target），不参与本工程编译/链接
    if(is_subcmake)
        if(NOT subcmake_source)
            set(subcmake_source "Program/${module_name}")
        endif()
        if(NOT subcmake_build)
            set(subcmake_build "Program/${module_name}/build")
        endif()
        if(NOT subcmake_generator)
            set(subcmake_generator "Ninja")
        endif()
        if(NOT subcmake_target)
            set(subcmake_target "all")
        endif()

        # 分流：不同配置写到不同 build 目录，避免 Debug/Release 相互污染
        set(_subcmake_src "${CMAKE_SOURCE_DIR}/${subcmake_source}")
        set(_subcmake_bld_base "${CMAKE_SOURCE_DIR}/${subcmake_build}")
        set(_subcmake_bld "${_subcmake_bld_base}/$<CONFIG>")

        # NOTE: add_custom_target() does not support EXCLUDE_FROM_ALL.
        # Not adding ALL already means it won't build by default.
        # Attach SOURCES so Visual Studio shows the files under this single target (no extra *_files target).
        add_custom_target(${name}
            COMMAND ${CMAKE_COMMAND} -S "${_subcmake_src}" -B "${_subcmake_bld}" -G "${subcmake_generator}"
                    -DCMAKE_BUILD_TYPE=$<CONFIG>
                    -DSMALLWASM_DEBUG=$<IF:$<CONFIG:Debug>,ON,OFF>
                    -DLLVM_ROOT=$ENV{LLVM_ROOT}
                    -DWASMOPT=$ENV{WASMOPT}
                    ${subcmake_configure_args}
            COMMAND ${CMAKE_COMMAND} --build "${_subcmake_bld}" --target "${subcmake_target}"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            VERBATIM
            SOURCES ${files} "${module_file}"
        )
        if(NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> SUBCMAKE")
        endif()

        # VS 文件夹层级
        get_vs_folder_path(${module_file} vs_folder_path)
        if(vs_folder_path STREQUAL "Modules/Program")
            set(vs_folder_path "Program")
        endif()
        set_target_properties(${name} PROPERTIES FOLDER "${vs_folder_path}")

        # VS 过滤器：让 smallwasm 的文件在 VS 中按目录树展示（src/ui, web 等）
        if(MSVC)
            if(files)
                set(_subcmake_abs_files "")
                foreach(_f ${files})
                    list(APPEND _subcmake_abs_files "${CMAKE_SOURCE_DIR}/${_f}")
                endforeach()
                # Use the sub-project root so filters are relative (src/..., web/...) instead of full repo paths.
                source_group(TREE "${CMAKE_SOURCE_DIR}/${subcmake_source}" FILES ${_subcmake_abs_files})
            endif()
            # Add JSON config to a specific group
            source_group("Config" FILES "${module_file}")
        endif()

        # NOTE: no ${name}_files target; VS browsing uses the SOURCES attached to ${name}.

        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "Module: ${name}")
        endif()
        continue()
    endif()

    # 检查模块类型
    check_module_type("${types}" "exe" is_executable)
    check_module_type("${types}" "static" is_static)
    check_module_type("${types}" "shared" is_shared)
    check_module_type("${types}" "third" is_third)
    check_module_type("${types}" "interface" is_interface_type)
    check_module_type("${types}" "lib" is_lib)

    set(is_interface FALSE)
    set(has_sources TRUE)

    # 根据模块类型创建目标
    if(is_executable)
        add_executable(${name} EXCLUDE_FROM_ALL)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> EXECUTABLE (Excluded from ALL)")
        endif()
    elseif(is_third)
        # 方案：使用 STATIC 库 + Dummy 源文件
        # 这样既能在 VS 中显示文件（无需 _View 后缀），又能生成 .lib 文件避免 LNK1104 错误
        
        # 创建一个空的 dummy 源文件，强制生成 .lib
        set(_dummy_file "${CMAKE_BINARY_DIR}/module_dummy/${name}_dummy.cpp")
        if(NOT EXISTS "${_dummy_file}")
            file(WRITE "${_dummy_file}" "// Dummy file to force static lib generation for ${name}\n")
        endif()

        set(_all_third_files ${header_files} ${source_files})
        add_library(${name} STATIC ${_all_third_files} "${_dummy_file}")
        set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)
        
        # 标记真实文件为只读头文件，防止被编译（避免符号冲突）
        if(_all_third_files)
            foreach(f ${_all_third_files})
                set_source_files_properties(${f} PROPERTIES HEADER_FILE_ONLY TRUE)
            endforeach()
        endif()

        set(is_interface FALSE)
        set(has_sources FALSE)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> STATIC (third party)")
        endif()
    elseif(is_interface_type)
        add_library(${name} INTERFACE)
        set(is_interface TRUE)
        set(has_sources FALSE)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> INTERFACE")
        endif()
    elseif(NOT source_files AND header_files)
        # 将 header-only 模块也作为 STATIC 库处理，以便在 VS 中正确显示文件结构
        # 且支持 PRIVATE 添加配置文件而不污染依赖者
        add_library(${name} STATIC)
        set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)
        set(is_interface FALSE) # 视为普通库
        set(has_sources FALSE)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> STATIC (header only)")
        endif()
    elseif(is_static)
        add_library(${name} STATIC)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> STATIC")
        endif()
    elseif(is_shared)
        add_library(${name} SHARED)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> SHARED")
        endif()
    elseif(is_lib OR source_files)
        add_library(${name} STATIC)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> STATIC")
        endif()
    else()
        add_library(${name} SHARED)
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  ${name} -> SHARED (default)")
        endif()
    endif()

    # 添加源文件和头文件
    if(is_third)
        # 第三方库文件已通过 Custom Target 添加显示，此处无需处理
    elseif(is_interface)
        target_sources(${name} INTERFACE ${header_files})
        should_show_module(${name})
        if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
            message(STATUS "  添加接口库文件: ${header_files}")
        endif()
    else()
        if(header_files)
            target_sources(${name} PUBLIC ${header_files})
        endif()
        if(source_files)
            target_sources(${name} PRIVATE ${source_files})
        endif()
    endif()



    # 在非模块模式下，将 .ixx 文件设置为普通头文件，不作为模块接口单元
    if(NOT SHINE_BUILD_MODULE AND header_files)
        foreach(file ${header_files})
            if(file MATCHES "\\.ixx$")
                set_source_files_properties(${CMAKE_SOURCE_DIR}/${file} PROPERTIES
                    CXX_MODULE OFF
                    CXX_MODULE_TYPE "None"
                    HEADER_FILE_ONLY TRUE
                )
                # 在非模块模式下，为 .ixx 文件添加禁用模块的编译选项
                if(MSVC)
                    set_source_files_properties(${CMAKE_SOURCE_DIR}/${file} PROPERTIES
                        COMPILE_FLAGS "/Zc:twoPhase-"
                    )
                endif()
            endif()
        endforeach()
    endif()

    # 配置目标属性
    if(is_interface)
        target_include_directories(${name} INTERFACE ${COMMON_INCLUDE_DIRS})
        # INTERFACE 库也需要设置链接目录，以便第三方库文件能被找到
        target_link_directories(${name} INTERFACE ${CMAKE_SOURCE_DIR}/exe ${CMAKE_SOURCE_DIR}/src/third/lib)
    else()
        set_module_properties(${name})
        target_include_directories(${name} PRIVATE ${COMMON_INCLUDE_DIRS})
        target_link_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/exe ${CMAKE_SOURCE_DIR}/src/third/lib)
    endif()

    # 设置VS文件夹层级
    if(is_executable)
        set_target_properties(${name} PROPERTIES FOLDER "Program")
    else()
        get_vs_folder_path(${module_file} vs_folder_path)
        set_target_properties(${name} PROPERTIES FOLDER "${vs_folder_path}")
    endif()

    # 将模块的 JSON 配置文件添加到目标中
    if(module_file AND EXISTS "${module_file}")
        if(is_interface)
            target_sources(${name} INTERFACE "${module_file}")
        else()
            target_sources(${name} PRIVATE "${module_file}")
        endif()

        set_source_files_properties("${module_file}" PROPERTIES
            HEADER_FILE_ONLY TRUE
            VS_TOOL_OVERRIDE "Text"
        )
        if(MSVC)
            source_group("Config" FILES "${module_file}")
        endif()
    endif()

    # VS过滤器设置 - 只有当模块不是 MainEngine 时（MainEngine 下面会单独处理），
    # 或者对于想要完全自定义目录结构的模块，可以移除这段逻辑。
    # 这里我们修改逻辑：如果设置了 VS_NO_SOURCE_GROUP 属性，则不生成 source_group
    get_target_property(_no_src_group ${name} VS_NO_SOURCE_GROUP)
    if(MSVC AND (source_files OR header_files) AND NOT _no_src_group)
        # 如果是 MainEngine，我们希望使用实际的目录结构，而不是强制归类到 src/include
        if(NOT name STREQUAL "MainEngine")
            # 对于其他模块，如果希望保留原始目录结构，可以注释掉下面这部分
            # 或者只对确实需要分离 include/src 的模块启用
            
            # 暂时注释掉强制分组，让 VS 按照实际文件目录显示
            # foreach(header_file ${header_files})
            #    source_group("include" FILES "${CMAKE_SOURCE_DIR}/${header_file}")
            # endforeach()
            # foreach(src_file ${source_files})
            #    source_group("src" FILES "${CMAKE_SOURCE_DIR}/${src_file}")
            # endforeach()
            
            # 使用 TREE 方式自动按目录结构分组
            set(_all_files ${header_files} ${source_files})
            set(_abs_files "")
            foreach(_f ${_all_files})
                get_filename_component(_abs "${CMAKE_SOURCE_DIR}/${_f}" ABSOLUTE)
                list(APPEND _abs_files "${_abs}")
            endforeach()
            
            # 找到这些文件的公共根目录，通常是 src/module_name 或 src/third/module_name
            # 这里简单起见，我们以 CMAKE_SOURCE_DIR 为根，这样文件会显示在 src/... 下
            # 如果想让它们直接显示在项目根目录下，可以调整 PREFIX
            source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${_abs_files})
        endif()
        set_target_properties(${name} PROPERTIES VS_SCC_LOCALPATH "")
    endif()

    # 可执行文件特殊配置
    if(is_executable)
        target_compile_options(${name} PRIVATE $<$<CONFIG:Release>:/GL>)
        target_link_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/exe ${CMAKE_SOURCE_DIR}/src/third/lib)
        target_link_libraries(${name} PRIVATE
            $<$<CONFIG:Debug>:glew32d>
            $<$<CONFIG:Release>:glew32>
            $<$<PLATFORM_ID:Windows>:opengl32;glu32;dxgi>
        )
        set_target_properties(${name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/exe"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/exe"
            OUTPUT_NAME_DEBUG "${name}d"
            OUTPUT_NAME_RELEASE "${name}"
            FOLDER "Program"
        )
    endif()

    # 设置编译定义
    set_target_defines(${name} "${defines}" "${platforms}" "${types}" ${is_interface} ${is_third})

    # 链接第三方库
    link_third_party_libs(${name} "${third_libs_debug}" "${third_libs_release}" ${is_interface} ${is_third})

    # 设置导出宏
    if(NOT name STREQUAL "fmt" AND source_files)
        string(TOUPPER ${name} name_upper)
        if(is_static)
            target_compile_definitions(${name} PUBLIC "${name_upper}_API=")
        else()
            target_compile_definitions(${name} PRIVATE "${name_upper}_API=__declspec(dllexport)")
            target_compile_definitions(${name} INTERFACE "${name_upper}_API=__declspec(dllimport)")
            set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        endif()
    endif()

    # 设置C++标准
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    should_show_module(${name})
    if(SHOULD_SHOW AND NOT SHINE_QUIET_MODULE_MESSAGES)
        message(STATUS "Module: ${name}")
    endif()
endforeach()

# In WASM dispatch-only mode we stop here: only subcmake targets are needed.
if(SHINE_WASM_DISPATCH_ONLY)
    return()
endif()

# ================================
# 依赖关系解析
# ================================
function(resolve_all_dependencies module_name result_deps)
    set(all_deps "")
    set(to_process ${module_name})
    set(processed "")

    while(to_process)
        list(POP_FRONT to_process current)
        if(${current} IN_LIST processed)
            continue()
        endif()
        list(APPEND processed ${current})

        get_property(current_deps GLOBAL PROPERTY "MODULE_${current}_DEPS")
        if(current_deps)
            foreach(dep ${current_deps})
                if(NOT ${dep} IN_LIST all_deps)
                    list(APPEND all_deps ${dep})
                endif()
                if(NOT ${dep} IN_LIST to_process AND NOT ${dep} IN_LIST processed)
                    list(APPEND to_process ${dep})
                endif()
            endforeach()
        endif()
    endwhile()

    set(${result_deps} ${all_deps} PARENT_SCOPE)
endfunction()

function(get_third_party_libs dep result_debug result_release)
    get_property(third_libs_debug GLOBAL PROPERTY "MODULE_${dep}_THIRD_LIBS_DEBUG")
    get_property(third_libs_release GLOBAL PROPERTY "MODULE_${dep}_THIRD_LIBS_RELEASE")
    set(${result_debug} ${third_libs_debug} PARENT_SCOPE)
    set(${result_release} ${third_libs_release} PARENT_SCOPE)
endfunction()

# ================================
# 添加依赖关系与链接
# ================================
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")
    get_property(direct_deps GLOBAL PROPERTY "MODULE_${module_name}_DEPS")

    if(TARGET ${name} AND direct_deps)
        resolve_all_dependencies(${module_name} all_deps)

        # 构造需要链接的集合：直接依赖 + 传递依赖（去重）
        set(to_link ${direct_deps})
        foreach(_tdep ${all_deps})
            list(FIND to_link ${_tdep} _found)
            if(_found EQUAL -1)
                list(APPEND to_link ${_tdep})
            endif()
        endforeach()

        # 为直接依赖添加构建顺序依赖
        foreach(dep ${direct_deps})
            if(TARGET ${dep})
                get_property(dep_files GLOBAL PROPERTY "MODULE_${dep}_FILES")
                if("${dep_files}" MATCHES "\\.(h|hpp|ixx)$")
                    add_dependencies(${name} ${dep})
                endif()
            endif()
        endforeach()

        # 链接依赖
        foreach(dep ${to_link})
            get_property(dep_files GLOBAL PROPERTY "MODULE_${dep}_FILES")
            get_property(dep_types GLOBAL PROPERTY "MODULE_${dep}_TYPES")

            # 检查依赖是否是 exe 类型，exe 类型不应该被当作库来链接
            check_module_type("${dep_types}" "exe" dep_is_executable)

            set(needs_link FALSE)
            if(NOT dep_is_executable AND (dep STREQUAL "fmt" OR "${dep_files}" MATCHES "\\.(cpp|ixx|cppm)$"))
                set(needs_link TRUE)
            endif()

            if(needs_link)
                # 获取依赖和目标是否是 INTERFACE 库的信息
                get_property(dep_is_interface GLOBAL PROPERTY "MODULE_${dep}_IS_INTERFACE")
                get_property(current_is_interface GLOBAL PROPERTY "MODULE_${module_name}_IS_INTERFACE")
                
                # 如果属性不存在，回退到检查类型
                if(NOT DEFINED dep_is_interface)
                    check_module_type("${dep_types}" "third" is_third_party)
                    if(is_third_party)
                        set(dep_is_interface TRUE)
                    else()
                        set(dep_is_interface FALSE)
                    endif()
                endif()
                
                if(NOT DEFINED current_is_interface)
                    check_module_type("${types}" "third" current_is_third)
                    if(current_is_third)
                        set(current_is_interface TRUE)
                    else()
                        set(current_is_interface FALSE)
                    endif()
                endif()
                
                if(TARGET ${dep})
                    if(dep_is_interface)
                        # 如果依赖是 INTERFACE 库，必须使用 INTERFACE 或 PUBLIC
                        if(current_is_interface)
                            # 当前目标也是 INTERFACE 库，使用 INTERFACE
                            target_link_libraries(${name} INTERFACE ${dep})
                        else()
                            # 当前目标是普通库，使用 PUBLIC（这样依赖会传递）
                            target_link_libraries(${name} PUBLIC ${dep})
                        endif()
                    else()
                        # 依赖是普通库
                        if(current_is_interface)
                            # 当前目标是 INTERFACE 库，使用 INTERFACE
                            target_link_libraries(${name} INTERFACE ${dep})
                        else()
                            # 当前目标是普通库，使用 PRIVATE
                            target_link_libraries(${name} PRIVATE ${dep})
                        endif()
                    endif()
                endif()
            endif()
        endforeach()
    endif()
endforeach()

# ================================
# 主引擎目标
# ================================
file(GLOB_RECURSE ENGINE_SOURCES
    "src/**/*.cpp"
    "src/**/*.h"
    "src/**/*.hpp"
    "src/**/*.inl"
    "src/**/*.ixx"
    "src/**/*.cppm"
    "src/ShineEngine.cpp"
)

# 排除第三方库和已编译到模块中的源文件
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "src/third/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/imgui/.*")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/tinygltf/.*")

# 收集所有模块的源文件路径并排除
set(EXCLUDE_ABS_PATHS "")
set(EXCLUDE_REL_PATHS "")
foreach(module_name ${MODULE_LIST})
    get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
    foreach(mod_src ${files})
        get_filename_component(abs_mod_src "${CMAKE_SOURCE_DIR}/${mod_src}" ABSOLUTE)
        file(TO_CMAKE_PATH "${abs_mod_src}" abs_mod_src_norm)
        list(APPEND EXCLUDE_ABS_PATHS "${abs_mod_src_norm}")
        list(APPEND EXCLUDE_ABS_PATHS "${abs_mod_src}")
        list(APPEND EXCLUDE_REL_PATHS "${mod_src}")
        file(TO_CMAKE_PATH "${mod_src}" mod_src_norm)
        list(APPEND EXCLUDE_REL_PATHS "${mod_src_norm}")
    endforeach()
endforeach()

# 排除模块文件 - 只排除模块目录下的文件，避免影响 MainEngine 的同名文件
set(FILTERED_SOURCES "")
foreach(src ${ENGINE_SOURCES})
    if(IS_ABSOLUTE "${src}")
        set(abs_src "${src}")
        file(RELATIVE_PATH rel_src "${CMAKE_SOURCE_DIR}" "${src}")
    else()
        get_filename_component(abs_src "${CMAKE_SOURCE_DIR}/${src}" ABSOLUTE)
        set(rel_src "${src}")
    endif()
    file(TO_CMAKE_PATH "${abs_src}" abs_src_norm)
    file(TO_CMAKE_PATH "${rel_src}" rel_src_norm)
    string(REPLACE "\\" "/" abs_src_unix "${abs_src_norm}")
    string(REPLACE "\\" "/" rel_src_unix "${rel_src_norm}")

    list(FIND EXCLUDE_ABS_PATHS "${abs_src_norm}" found_abs1)
    list(FIND EXCLUDE_ABS_PATHS "${abs_src}" found_abs2)
    list(FIND EXCLUDE_ABS_PATHS "${abs_src_unix}" found_abs3)
    list(FIND EXCLUDE_REL_PATHS "${rel_src_norm}" found_rel1)
    list(FIND EXCLUDE_REL_PATHS "${rel_src}" found_rel2)
    list(FIND EXCLUDE_REL_PATHS "${rel_src_unix}" found_rel3)

    set(should_exclude FALSE)
    if(found_abs1 GREATER_EQUAL 0 OR found_abs2 GREATER_EQUAL 0 OR found_abs3 GREATER_EQUAL 0 OR
       found_rel1 GREATER_EQUAL 0 OR found_rel2 GREATER_EQUAL 0 OR found_rel3 GREATER_EQUAL 0)
        set(should_exclude TRUE)
    endif()

    if(NOT should_exclude)
        list(APPEND FILTERED_SOURCES "${src}")
    endif()
endforeach()
set(ENGINE_SOURCES ${FILTERED_SOURCES})

# 创建主引擎目标
add_executable(MainEngine ${ENGINE_SOURCES})

if(SHINE_BUILD_EDITOR)
    target_compile_definitions(MainEngine PRIVATE BUILD_EDITOR)
endif()

target_include_directories(MainEngine PRIVATE ${COMMON_INCLUDE_DIRS})
target_link_directories(MainEngine PRIVATE
    ${CMAKE_SOURCE_DIR}/exe
    ${CMAKE_SOURCE_DIR}/src/third/lib
)

set_target_properties(MainEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/exe"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/exe"
    OUTPUT_NAME_DEBUG "MainEngined"
    OUTPUT_NAME_RELEASE "MainEngine"
)

# VS过滤器设置
if(MSVC)
    foreach(src ${ENGINE_SOURCES})
        get_filename_component(src_path "${src}" PATH)
        file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}" "${src_path}")
        string(REPLACE "/" "\\\\" group_path "${rel_path}")
        if(group_path STREQUAL "")
            set(group_path ".")
        endif()
        source_group("${group_path}" FILES "${src}")
    endforeach()

    set_target_properties(MainEngine PROPERTIES FOLDER "Engine")

    foreach(module_name ${MODULE_LIST})
        get_property(module_target_name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
        if(TARGET ${module_target_name})
            set_target_properties(${module_target_name} PROPERTIES VS_SCC_LOCALPATH "")
        endif()
    endforeach()
endif()

# 链接所有模块（排除可执行文件类型）和第三方库
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")
    
    check_module_type("${types}" "exe" is_executable)
    check_module_type("${types}" "subcmake" is_subcmake)
    check_module_type("${types}" "third" is_third_party)
    
    if(NOT is_executable AND NOT is_subcmake AND TARGET ${name})
        add_dependencies(MainEngine ${name})
        get_property(is_interface GLOBAL PROPERTY "MODULE_${module_name}_IS_INTERFACE")
        if(is_third_party OR is_interface)
            target_link_libraries(MainEngine PUBLIC ${name})
        else()
            target_link_libraries(MainEngine PRIVATE ${name})
        endif()
    endif()
    
    # 链接第三方库的静态库文件
    if(is_third_party)
        get_property(third_libs_debug GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG")
        get_property(third_libs_release GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE")
        foreach(lib ${third_libs_debug})
            target_link_libraries(MainEngine PRIVATE 
                $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
        endforeach()
        foreach(lib ${third_libs_release})
            target_link_libraries(MainEngine PRIVATE 
                $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
        endforeach()
    endif()
endforeach()

target_link_libraries(MainEngine PRIVATE
    $<$<CONFIG:Debug>:glew32d>
    $<$<CONFIG:Release>:glew32>
    $<$<PLATFORM_ID:Windows>:opengl32;glu32;dxgi>
)

# 设置运行时环境变量
if(WIN32)
    set_target_properties(MainEngine PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/exe/module;${CMAKE_BINARY_DIR}/exe;${CMAKE_SOURCE_DIR}/src/third/lib;$ENV{PATH}"
    )
endif()

# ================================
# 复制第三方库 DLL 文件到输出目录
if(WIN32 AND TARGET MainEngine)
    add_custom_command(TARGET MainEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/third/lib/glew32.dll"
            "${CMAKE_SOURCE_DIR}/exe/glew32.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/src/third/lib/glew32d.dll"
            "${CMAKE_SOURCE_DIR}/exe/glew32d.dll"
        COMMENT "Copying GLEW DLL files to exe directory..."
    )
endif()



# ================================
# 脚本文件目标（VS中查看脚本）
# ================================
file(GLOB_RECURSE SCRIPT_FILES "${CMAKE_SOURCE_DIR}/script/*.ts" "${CMAKE_SOURCE_DIR}/script/*.js" "${CMAKE_SOURCE_DIR}/build/script/*.js")
if(SCRIPT_FILES)
    add_custom_target(Scripts SOURCES ${SCRIPT_FILES})
    set_target_properties(Scripts PROPERTIES
        FOLDER "Scripts"
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
    )

    foreach(script_file ${SCRIPT_FILES})
        set_source_files_properties("${script_file}" PROPERTIES
            VS_TOOL_OVERRIDE "Text"
            HEADER_FILE_ONLY TRUE
        )
    endforeach()

    if(MSVC)
        foreach(script_file ${SCRIPT_FILES})
            file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}/script" "${script_file}")
            get_filename_component(file_dir "${rel_path}" DIRECTORY)
            if(file_dir AND NOT file_dir STREQUAL ".")
                string(REPLACE "/" "\\" group_path "${file_dir}")
            else()
                set(group_path "Scripts")
            endif()
            source_group("${group_path}" FILES "${script_file}")
        endforeach()
    endif()
endif()

# ================================
# 构建信息输出
# ================================
message(STATUS "=== Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
if(WIN32)
    message(STATUS "Architecture: ${CMAKE_GENERATOR_PLATFORM}")
endif()
message(STATUS "C++20 Modules: ${SHINE_BUILD_MODULE}")
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "==========================")

# ================================
# 测试系统
# ================================
option(BUILD_TESTS "Build the performance test system" ON)
if(BUILD_TESTS)
    message(STATUS "Including test system...")
    if(EXISTS "${CMAKE_SOURCE_DIR}/test/CMakeLists.txt")
        add_subdirectory(test)
    else()
        message(STATUS "Test directory not found, skipping")
    endif()
endif()

