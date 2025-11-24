cmake_minimum_required(VERSION 3.28)
project(ShineEngine VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

# 全局编译定义
add_compile_definitions(
    SHINE_OPENGL
    STB_IMAGE_IMPLEMENTATION
    $<$<CONFIG:Release>:NDEBUG>
    $<$<BOOL:${EMSCRIPTEN}>:__EMSCRIPTEN__>
    $<$<BOOL:${EMSCRIPTEN}>:SHINE_IMPORT_STD>
)

# 统一的包含目录
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/util
    ${CMAKE_SOURCE_DIR}/src/third
    ${CMAKE_SOURCE_DIR}/src/third/imgui
    ${CMAKE_SOURCE_DIR}/src/third/imgui/backends
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
    find_module_configs(ALL_MODULE_FILES "Module")
    categorize_modules_by_type(${ALL_MODULE_FILES})
    set(MODULE_TYPES lib static shared exe third)
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
        parse_json_array(${json_content} "deps" deps_list)
        parse_json_array(${json_content} "defines" defines_list)
        parse_json_array(${json_content} "dirs" dirs_list)
        parse_json_array(${json_content} "platform" platform_list)

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

        # 缓存到全局属性
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_NAME" ${name})
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_FILES" "${files_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_DEPS" "${deps_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_DEFINES" "${defines_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG" "${third_lib_debug_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE" "${third_lib_release_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_PLATFORMS" "${platform_list}")
        set_property(GLOBAL PROPERTY "MODULE_${module_name}_TYPES" "${type_list}")
        
        # 预判断是否是 INTERFACE 库（用于后续依赖链接）
        set(is_interface_predicted FALSE)
        if("${type_list}" MATCHES "third")
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
function(set_target_defines target_name defines platforms types is_interface)
    if(is_interface)
        set(scope INTERFACE)
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
endfunction()

# ================================
# 统一链接第三方库
# ================================
function(link_third_party_libs target_name third_libs_debug third_libs_release is_interface)
    if(is_interface)
        set(scope INTERFACE)
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

# ================================
# 创建所有模块目标
# ================================
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
    get_property(defines GLOBAL PROPERTY "MODULE_${module_name}_DEFINES")
    get_property(third_libs_debug GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG")
    get_property(third_libs_release GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE")
    get_property(platforms GLOBAL PROPERTY "MODULE_${module_name}_PLATFORMS")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")

    find_module_file(${module_name} module_file)

    if(NOT files)
        message(STATUS "Skip empty module: ${name}")
        continue()
    endif()

    # 分类文件类型
    set(header_files "")
    set(source_files "")
    foreach(file ${files})
        if(file MATCHES "\\.(h|hpp|ixx)$")
            list(APPEND header_files ${file})
        else()
            list(APPEND source_files ${file})
        endif()
    endforeach()

    # 检查模块类型
    check_module_type("${types}" "exe" is_executable)
    check_module_type("${types}" "static" is_static)
    check_module_type("${types}" "shared" is_shared)
    check_module_type("${types}" "third" is_third)
    check_module_type("${types}" "lib" is_lib)

    set(is_interface FALSE)
    set(has_sources TRUE)

    # 根据模块类型创建目标
    if(is_executable)
        add_executable(${name} EXCLUDE_FROM_ALL)
        message(STATUS "  ${name} -> EXECUTABLE (Excluded from ALL)")
    elseif(is_static)
        add_library(${name} STATIC)
        message(STATUS "  ${name} -> STATIC")
    elseif(is_shared)
        add_library(${name} SHARED)
        message(STATUS "  ${name} -> SHARED")
    elseif(is_third)
        add_library(${name} INTERFACE)
        set(is_interface TRUE)
        set(has_sources FALSE)
        message(STATUS "  ${name} -> INTERFACE (third party)")
    elseif(NOT source_files AND header_files)
        add_library(${name} INTERFACE)
        set(is_interface TRUE)
        set(has_sources FALSE)
        message(STATUS "  ${name} -> INTERFACE (header only)")
    elseif(is_lib OR source_files)
        add_library(${name} STATIC)
        message(STATUS "  ${name} -> STATIC")
    else()
        add_library(${name} SHARED)
        message(STATUS "  ${name} -> SHARED (default)")
    endif()

    # 添加源文件和头文件
    if(is_third)
        message(STATUS "  添加第三方库文件: 无源文件")
    elseif(is_interface)
        target_sources(${name} INTERFACE ${header_files})
        message(STATUS "  添加接口库文件: ${header_files}")
    else()
        if(header_files)
            target_sources(${name} PUBLIC ${header_files})
        endif()
        if(source_files)
            target_sources(${name} PRIVATE ${source_files})
        endif()
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
        set_target_properties(${name} PROPERTIES FOLDER "Launchers")
    else()
        get_vs_folder_path(${module_file} vs_folder_path)
        set_target_properties(${name} PROPERTIES FOLDER "${vs_folder_path}")
        if(MSVC AND is_third)
            add_custom_target(${name}_files SOURCES ${header_files})
            set_target_properties(${name}_files PROPERTIES FOLDER "${vs_folder_path}")
        endif()
    endif()

    # VS过滤器设置
    if(MSVC AND NOT is_third AND (source_files OR header_files))
        foreach(header_file ${header_files})
            source_group("include" FILES "${CMAKE_SOURCE_DIR}/${header_file}")
        endforeach()
        foreach(src_file ${source_files})
            source_group("src" FILES "${CMAKE_SOURCE_DIR}/${src_file}")
        endforeach()
        set_target_properties(${name} PROPERTIES VS_SCC_LOCALPATH "")
    endif()

    # 可执行文件特殊配置
    if(is_executable)
        target_compile_options(${name} PRIVATE $<$<CONFIG:Release>:/GL>)
        target_link_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/exe)
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
            FOLDER "Launchers"
        )
    endif()

    # 设置编译定义
    set_target_defines(${name} "${defines}" "${platforms}" "${types}" ${is_interface})

    # 链接第三方库
    link_third_party_libs(${name} "${third_libs_debug}" "${third_libs_release}" ${is_interface})

    # 设置导出宏
    if(NOT name STREQUAL "fmt" AND source_files)
        string(TOUPPER ${name} name_upper)
        if(is_static)
            target_compile_definitions(${name} PUBLIC "EXPORT_${name_upper}=")
        else()
            target_compile_definitions(${name} PRIVATE "EXPORT_${name_upper}=__declspec(dllexport)")
            target_compile_definitions(${name} INTERFACE "EXPORT_${name_upper}=__declspec(dllimport)")
            set_target_properties(${name} PROPERTIES DEFINE_SYMBOL "")
        endif()
    endif()

    # 设置C++标准
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    message(STATUS "Module: ${name}")
endforeach()

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

            set(needs_link FALSE)
            if(dep STREQUAL "fmt" OR "${dep_files}" MATCHES "\\.(cpp|ixx|cppm)$")
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
# 转换为绝对路径进行比较，避免路径格式问题
set(EXCLUDE_ABS_PATHS "")
set(EXCLUDE_REL_PATHS "")
foreach(module_name ${MODULE_LIST})
    get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
    foreach(mod_src ${files})
        # 转换为绝对路径
        get_filename_component(abs_mod_src "${CMAKE_SOURCE_DIR}/${mod_src}" ABSOLUTE)
        # 规范化路径格式（统一使用正斜杠）
        file(TO_CMAKE_PATH "${abs_mod_src}" abs_mod_src_norm)
        list(APPEND EXCLUDE_ABS_PATHS "${abs_mod_src_norm}")
        list(APPEND EXCLUDE_ABS_PATHS "${abs_mod_src}")
        # 也添加相对路径
        list(APPEND EXCLUDE_REL_PATHS "${mod_src}")
        file(TO_CMAKE_PATH "${mod_src}" mod_src_norm)
        list(APPEND EXCLUDE_REL_PATHS "${mod_src_norm}")
    endforeach()
endforeach()

# 排除模块文件
set(FILTERED_SOURCES "")
foreach(src ${ENGINE_SOURCES})
    # 转换为绝对路径并规范化
    if(IS_ABSOLUTE "${src}")
        set(abs_src "${src}")
        file(RELATIVE_PATH rel_src "${CMAKE_SOURCE_DIR}" "${src}")
    else()
        get_filename_component(abs_src "${CMAKE_SOURCE_DIR}/${src}" ABSOLUTE)
        set(rel_src "${src}")
    endif()
    # 规范化路径格式（统一使用正斜杠）
    file(TO_CMAKE_PATH "${abs_src}" abs_src_norm)
    file(TO_CMAKE_PATH "${rel_src}" rel_src_norm)
    # 也尝试将反斜杠替换为正斜杠
    string(REPLACE "\\" "/" abs_src_unix "${abs_src_norm}")
    string(REPLACE "\\" "/" rel_src_unix "${rel_src_norm}")
    # 检查是否在排除列表中（尝试多种格式）
    list(FIND EXCLUDE_ABS_PATHS "${abs_src_norm}" found_abs1)
    list(FIND EXCLUDE_ABS_PATHS "${abs_src}" found_abs2)
    list(FIND EXCLUDE_ABS_PATHS "${abs_src_unix}" found_abs3)
    list(FIND EXCLUDE_REL_PATHS "${rel_src_norm}" found_rel1)
    list(FIND EXCLUDE_REL_PATHS "${rel_src}" found_rel2)
    list(FIND EXCLUDE_REL_PATHS "${rel_src_unix}" found_rel3)
    # 如果找到匹配（任何格式），则排除该文件
    # 注意：list(FIND) 返回索引（>=0）表示找到，-1 表示未找到
    set(should_exclude FALSE)
    if(found_abs1 GREATER_EQUAL 0 OR found_abs2 GREATER_EQUAL 0 OR found_abs3 GREATER_EQUAL 0 OR
       found_rel1 GREATER_EQUAL 0 OR found_rel2 GREATER_EQUAL 0 OR found_rel3 GREATER_EQUAL 0)
        set(should_exclude TRUE)
    endif()
    # 如果找到匹配（任何格式），则排除该文件
    if(NOT should_exclude)
        list(APPEND FILTERED_SOURCES "${src}")
    endif()
endforeach()
set(ENGINE_SOURCES ${FILTERED_SOURCES})

# 最终检查：直接移除模块文件（使用 list(REMOVE_ITEM)）
foreach(module_name ${MODULE_LIST})
    get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
    foreach(mod_src ${files})
        get_filename_component(abs_mod_src "${CMAKE_SOURCE_DIR}/${mod_src}" ABSOLUTE)
        file(TO_CMAKE_PATH "${abs_mod_src}" abs_mod_src_norm)
        # 尝试多种路径格式移除
        list(REMOVE_ITEM ENGINE_SOURCES "${abs_mod_src_norm}")
        list(REMOVE_ITEM ENGINE_SOURCES "${abs_mod_src}")
        list(REMOVE_ITEM ENGINE_SOURCES "${mod_src}")
        # 也尝试 Unix 风格的路径
        string(REPLACE "\\" "/" abs_mod_src_unix "${abs_mod_src_norm}")
        list(REMOVE_ITEM ENGINE_SOURCES "${abs_mod_src_unix}")
        # 也尝试匹配文件名（不区分路径）
        get_filename_component(filename "${mod_src}" NAME)
        foreach(src ${ENGINE_SOURCES})
            get_filename_component(src_filename "${src}" NAME)
            if("${src_filename}" STREQUAL "${filename}")
                list(REMOVE_ITEM ENGINE_SOURCES "${src}")
            endif()
        endforeach()
    endforeach()
endforeach()
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]gltfLoader\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]loader\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]lodepng\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]quat\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]rotator\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]vector\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]vector2\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]obj\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]png\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]base64[/\\\\]base64\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]byte_convert\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]file_util\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]string_util\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]url_util\\.h$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*[/\\\\]timer[/\\\\].*\\.h$")

# 创建主引擎目标
# 注意：必须确保 ENGINE_SOURCES 中不包含模块文件
add_executable(MainEngine ${ENGINE_SOURCES})

# 双重保险：只对 MainEngine 目标中的模块文件设置属性以隐藏它们
# 注意：不能全局设置模块文件属性，否则会阻止模块本身的构建
if(TARGET MainEngine)
    get_target_property(target_sources MainEngine SOURCES)
    if(target_sources)
        foreach(module_name ${MODULE_LIST})
            get_property(files GLOBAL PROPERTY "MODULE_${module_name}_FILES")
            foreach(mod_src ${files})
                get_filename_component(filename "${mod_src}" NAME)
                # 只对 MainEngine 目标中的文件设置属性
                foreach(src ${target_sources})
                    get_filename_component(src_filename "${src}" NAME)
                    if("${src_filename}" STREQUAL "${filename}")
                        set_source_files_properties("${src}" PROPERTIES 
                            HEADER_FILE_ONLY TRUE
                            VS_SETTINGS "ExcludeFromBuild=true"
                        )
                    endif()
                endforeach()
            endforeach()
        endforeach()
    endif()
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

# 链接所有模块（排除可执行文件类型）
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")
    
    check_module_type("${types}" "exe" is_executable)
    if(NOT is_executable AND TARGET ${name})
        add_dependencies(MainEngine ${name})
        # 对于 INTERFACE 库（如 fmt），使用 PUBLIC 链接以确保第三方库链接能传递
        check_module_type("${types}" "third" is_third_party)
        get_property(is_interface GLOBAL PROPERTY "MODULE_${module_name}_IS_INTERFACE")
        if(is_third_party OR is_interface)
            target_link_libraries(MainEngine PUBLIC ${name})
        else()
            target_link_libraries(MainEngine PRIVATE ${name})
        endif()
    endif()
endforeach()

# 确保 MainEngine 也直接链接所有第三方库的链接配置
# 使用完整路径，与 link_third_party_libs 函数保持一致
foreach(module_name ${MODULE_LIST})
    get_property(name GLOBAL PROPERTY "MODULE_${module_name}_NAME")
    get_property(types GLOBAL PROPERTY "MODULE_${module_name}_TYPES")
    check_module_type("${types}" "third" is_third_party)
    if(is_third_party)
        get_property(third_libs_debug GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_DEBUG")
        get_property(third_libs_release GLOBAL PROPERTY "MODULE_${module_name}_THIRD_LIBS_RELEASE")
        # 调试输出：验证解析是否成功（已确认工作正常，可注释掉）
        # message(STATUS "MainEngine linking third-party libs for ${name}: Debug=${third_libs_debug}, Release=${third_libs_release}")
        if(third_libs_debug)
            foreach(lib ${third_libs_debug})
                # 使用完整路径，与 link_third_party_libs 函数保持一致
                target_link_libraries(MainEngine PRIVATE 
                    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
            endforeach()
        endif()
        if(third_libs_release)
            foreach(lib ${third_libs_release})
                # 使用完整路径，与 link_third_party_libs 函数保持一致
                target_link_libraries(MainEngine PRIVATE 
                    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/src/third/lib/${lib}>)
            endforeach()
        endif()
    endif()
endforeach()

target_link_libraries(MainEngine PRIVATE
    $<$<CONFIG:Debug>:glew32d>
    $<$<CONFIG:Release>:glew32>
    $<$<PLATFORM_ID:Windows>:opengl32;glu32;dxgi;luajit;lua51>
)

# 设置运行时环境变量
if(WIN32)
    set_target_properties(MainEngine PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/exe/module;${CMAKE_BINARY_DIR}/exe;${CMAKE_SOURCE_DIR}/src/third/lib;$ENV{PATH}"
    )
endif()

# ================================
# 模块配置文件目标（VS中编辑JSON）
# ================================
get_property(MODULE_JSON_FILES GLOBAL PROPERTY "ALL_MODULE_FILES")
add_custom_target(ModuleConfigs SOURCES ${MODULE_JSON_FILES})
set_target_properties(ModuleConfigs PROPERTIES
    FOLDER ""
    EXCLUDE_FROM_ALL TRUE
    EXCLUDE_FROM_DEFAULT_BUILD TRUE
)

foreach(json_file ${MODULE_JSON_FILES})
    set_source_files_properties("${json_file}" PROPERTIES
        VS_TOOL_OVERRIDE "Text"
        HEADER_FILE_ONLY TRUE
    )
endforeach()

if(MSVC)
    foreach(json_file ${MODULE_JSON_FILES})
        file(RELATIVE_PATH rel_path "${CMAKE_SOURCE_DIR}/Module" "${json_file}")
        get_filename_component(file_dir "${rel_path}" DIRECTORY)
        if(file_dir AND NOT file_dir STREQUAL ".")
            string(REPLACE "/" "\\" group_path "${file_dir}")
        else()
            set(group_path "")
        endif()
        source_group("${group_path}" FILES "${json_file}")
    endforeach()
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

