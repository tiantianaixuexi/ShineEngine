cmake_minimum_required(VERSION 3.28)
project(smallwasm LANGUAGES C CXX)

# =========================================================

# Options

# =========================================================

option(SMALLWASM_ANALYZE "Enable wasm debug info for analysis (names + line tables)" OFF)
option(SMALLWASM_DEBUG_WEB "Enable web debug HUD (GPU timing, stats, overlay)" ON)

# =========================================================

# Toolchain paths

# =========================================================

set(LLVM_ROOT "" CACHE PATH "Path to LLVM root (contains bin/clang(.exe) and bin/wasm-ld(.exe))")
set(WASMOPT   "" CACHE FILEPATH "Path to wasm-opt(.exe) (optional)")

find_program(SMALLWASM_CLANG NAMES clang clang.exe
HINTS "${LLVM_ROOT}/bin" "${LLVM_ROOT}"
REQUIRED
)
find_program(SMALLWASM_WASM_LD NAMES wasm-ld wasm-ld.exe
HINTS "${LLVM_ROOT}/bin" "${LLVM_ROOT}"
REQUIRED
)

# =========================================================

# Output directories

# =========================================================

set(DIST_DIR "${CMAKE_CURRENT_LIST_DIR}/web/dist" CACHE PATH "Deploy directory")
file(MAKE_DIRECTORY "${DIST_DIR}")

# Preprocess web files (index.js, index.html) to strip debug code
function(preprocess_web_file input output is_debug)
    file(READ "${input}" CONTENT)
    if(NOT is_debug)
        set(CURRENT_CONTENT "${CONTENT}")
        
        # Remove JS blocks: // #if DEBUG ... // #endif
        while(TRUE)
            string(FIND "${CURRENT_CONTENT}" "// #if DEBUG" POS_START)
            if(POS_START EQUAL -1)
                break()
            endif()
            
            string(SUBSTRING "${CURRENT_CONTENT}" ${POS_START} -1 REST)
            string(FIND "${REST}" "// #endif" POS_END_REL)
            if(POS_END_REL EQUAL -1)
                message(WARNING "Unmatched // #if DEBUG in ${input}")
                break()
            endif()
            
            # Length of "// #endif" is 9
            math(EXPR REMOVE_LEN "${POS_END_REL} + 9")
            
            string(SUBSTRING "${CURRENT_CONTENT}" 0 ${POS_START} PART1)
            math(EXPR PART2_START "${POS_START} + ${REMOVE_LEN}")
            string(SUBSTRING "${CURRENT_CONTENT}" ${PART2_START} -1 PART2)
            set(CURRENT_CONTENT "${PART1}${PART2}")
        endwhile()

        # Remove HTML blocks: <!-- #if DEBUG --> ... <!-- #endif -->
        while(TRUE)
            string(FIND "${CURRENT_CONTENT}" "<!-- #if DEBUG -->" POS_START)
            if(POS_START EQUAL -1)
                break()
            endif()
            
            string(SUBSTRING "${CURRENT_CONTENT}" ${POS_START} -1 REST)
            string(FIND "${REST}" "<!-- #endif -->" POS_END_REL)
            if(POS_END_REL EQUAL -1)
                message(WARNING "Unmatched <!-- #if DEBUG --> in ${input}")
                break()
            endif()
            
            # Length of "<!-- #endif -->" is 15
            math(EXPR REMOVE_LEN "${POS_END_REL} + 15")
            
            string(SUBSTRING "${CURRENT_CONTENT}" 0 ${POS_START} PART1)
            math(EXPR PART2_START "${POS_START} + ${REMOVE_LEN}")
            string(SUBSTRING "${CURRENT_CONTENT}" ${PART2_START} -1 PART2)
            set(CURRENT_CONTENT "${PART1}${PART2}")
        endwhile()
        
        set(CONTENT "${CURRENT_CONTENT}")
    endif()
    
    # Fix paths: If we are moving index.js to dist/, we must update paths inside it.
    # dist/output.gl.wasm -> output.gl.wasm
    string(REPLACE "dist/output.gl.wasm" "output.gl.wasm" CONTENT "${CONTENT}")
    
    file(WRITE "${output}" "${CONTENT}")
endfunction()

preprocess_web_file("${CMAKE_CURRENT_LIST_DIR}/web/index.js" "${DIST_DIR}/index.js" ${SMALLWASM_DEBUG_WEB})
preprocess_web_file("${CMAKE_CURRENT_LIST_DIR}/web/index.html" "${DIST_DIR}/index.html" ${SMALLWASM_DEBUG_WEB})
# Also copy helper scripts if debug is on
if(SMALLWASM_DEBUG_WEB)
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/web/debug_utils.js" DESTINATION "${DIST_DIR}")
    file(COPY "${CMAKE_CURRENT_LIST_DIR}/web/stats_debug.js" DESTINATION "${DIST_DIR}")
endif()

set(TMP_DIR "${CMAKE_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${TMP_DIR}")

# =========================================================

# Common compile flags

# =========================================================

set(SMALLWASM_CXX_FLAGS
--target=wasm32
-Oz
-ffreestanding -nostdlib
-fno-exceptions -fno-rtti -fno-threadsafe-statics
-std=c++23
)

# ---- Analysis mode: keep function names + line info ----

if(SMALLWASM_ANALYZE)
list(APPEND SMALLWASM_CXX_FLAGS
-g
-gline-tables-only
-fdebug-compilation-dir=.
)
endif()

# =========================================================

# Link flags

# =========================================================

set(SMALLWASM_LD_FLAGS
--no-entry --export-memory --allow-undefined
--export=malloc --export=free
--export=init --export=frame --export=resize --export=pointer
--export=on_tex_loaded --export=on_tex_failed --export=gl_create_program_from_source
)

# ---- Strip control ----

if(SMALLWASM_ANALYZE)
set(SMALLWASM_LD_STRIP "")
else()
set(SMALLWASM_LD_STRIP "--strip-all")
endif()

string(JOIN " " SMALLWASM_LD_FLAGS_STR ${SMALLWASM_LD_FLAGS})

# =========================================================

# wasm-opt wrapper

# =========================================================

function(smallwasm_opt in_wasm out_wasm)
if(WASMOPT)
if(SMALLWASM_ANALYZE)
add_custom_command(
OUTPUT "${out_wasm}"
COMMAND "${WASMOPT}" -Oz "${in_wasm}" -o "${out_wasm}"
DEPENDS "${in_wasm}"
VERBATIM
)
else()
add_custom_command(
OUTPUT "${out_wasm}"
COMMAND "${WASMOPT}" -Oz --strip-dwarf --strip-producers --strip-names "${in_wasm}" -o "${out_wasm}"
DEPENDS "${in_wasm}"
VERBATIM
)
endif()
else()
add_custom_command(
OUTPUT "${out_wasm}"
COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${in_wasm}" "${out_wasm}"
DEPENDS "${in_wasm}"
VERBATIM
)
endif()
endfunction()

# =========================================================

# Sources

# =========================================================

set(GL_SRC   "${CMAKE_CURRENT_LIST_DIR}/src/gl_ctx.cpp")
set(GAPI_SRC "${CMAKE_CURRENT_LIST_DIR}/src/graphics/gl_api.cpp")
set(RT_SRC   "${CMAKE_CURRENT_LIST_DIR}/src/util/wasm_runtime.cpp")
set(OBJ_SRC  "${CMAKE_CURRENT_LIST_DIR}/src/game/object.cpp")
set(CMD_SRC  "${CMAKE_CURRENT_LIST_DIR}/src/graphics/wasm_command_buffer.cpp")
set(R2D_SRC  "${CMAKE_CURRENT_LIST_DIR}/src/graphics/renderer_2d.cpp")
set(TEX_SRC  "${CMAKE_CURRENT_LIST_DIR}/src/graphics/texture_manager.cpp")
set(UI_SRC   "${CMAKE_CURRENT_LIST_DIR}/src/ui/ui_manager.cpp")
set(ENG_SRC  "${CMAKE_CURRENT_LIST_DIR}/src/engine/engine.cpp")
set(DEMO_SRC "${CMAKE_CURRENT_LIST_DIR}/src/demo/demo_game.cpp")

set(GL_OBJ   "${CMAKE_BINARY_DIR}/gl_ctx.o")
set(GAPI_OBJ "${CMAKE_BINARY_DIR}/gl_api.o")
set(RT_OBJ   "${CMAKE_BINARY_DIR}/wasm_runtime.o")
set(OBJ_OBJ  "${CMAKE_BINARY_DIR}/object.o")
set(CMD_OBJ  "${CMAKE_BINARY_DIR}/wasm_command_buffer.o")
set(R2D_OBJ  "${CMAKE_BINARY_DIR}/renderer_2d.o")
set(TEX_OBJ  "${CMAKE_BINARY_DIR}/texture_manager.o")
set(UI_OBJ   "${CMAKE_BINARY_DIR}/ui_manager.o")
set(ENG_OBJ  "${CMAKE_BINARY_DIR}/engine.o")
set(DEMO_OBJ "${CMAKE_BINARY_DIR}/demo_game.o")

set(GL_RAW   "${TMP_DIR}/output.gl.raw.wasm")
set(GL_WASM  "${DIST_DIR}/output.gl.wasm")
set(GL_MAP   "${CMAKE_CURRENT_LIST_DIR}/output.gl.map")

# =========================================================

# Compile rules

# =========================================================


add_custom_command(OUTPUT ${GL_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${GL_SRC} -o ${GL_OBJ}
DEPENDS ${GL_SRC})
add_custom_command(OUTPUT ${GAPI_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${GAPI_SRC} -o ${GAPI_OBJ}
DEPENDS ${GAPI_SRC})
add_custom_command(OUTPUT ${RT_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${RT_SRC} -o ${RT_OBJ}
DEPENDS ${RT_SRC})
add_custom_command(OUTPUT ${OBJ_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${OBJ_SRC} -o ${OBJ_OBJ}
DEPENDS ${OBJ_SRC})
add_custom_command(OUTPUT ${CMD_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${CMD_SRC} -o ${CMD_OBJ}
DEPENDS ${CMD_SRC})
add_custom_command(OUTPUT ${R2D_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${R2D_SRC} -o ${R2D_OBJ}
DEPENDS ${R2D_SRC})
add_custom_command(OUTPUT ${TEX_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${TEX_SRC} -o ${TEX_OBJ}
DEPENDS ${TEX_SRC})
add_custom_command(OUTPUT ${UI_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${UI_SRC} -o ${UI_OBJ}
DEPENDS ${UI_SRC})
add_custom_command(OUTPUT ${ENG_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${ENG_SRC} -o ${ENG_OBJ}
DEPENDS ${ENG_SRC})
add_custom_command(OUTPUT ${DEMO_OBJ}
COMMAND ${SMALLWASM_CLANG} ${SMALLWASM_CXX_FLAGS} -c ${DEMO_SRC} -o ${DEMO_OBJ}
DEPENDS ${DEMO_SRC})

# =========================================================

# Link

# =========================================================

add_custom_command(
OUTPUT "${GL_RAW}"
COMMAND "${SMALLWASM_WASM_LD}" ${SMALLWASM_LD_FLAGS} ${SMALLWASM_LD_STRIP} --Map=${GL_MAP}
"${GL_OBJ}" "${GAPI_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}"
"${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}"
-o "${GL_RAW}"
DEPENDS "${GL_OBJ}" "${GAPI_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}"
"${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}"
VERBATIM
)

smallwasm_opt("${GL_RAW}" "${GL_WASM}")

add_custom_target(smallwasm ALL DEPENDS "${GL_WASM}")

message(STATUS "smallwasm analyze = ${SMALLWASM_ANALYZE}")
message(STATUS "smallwasm clang   = ${SMALLWASM_CLANG}")
message(STATUS "smallwasm wasm-ld = ${SMALLWASM_WASM_LD}")
message(STATUS "smallwasm wasm-opt= ${WASMOPT}")
message(STATUS "smallwasm dist dir= ${DIST_DIR}")
