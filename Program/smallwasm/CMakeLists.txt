cmake_minimum_required(VERSION 3.28)
project(smallwasm LANGUAGES C CXX)

# Minimal WebAssembly build without Emscripten.
# Produces:
# - web/dist/output.gl.wasm : smallest GL demo (strip-all + optional wasm-opt)
#
# Usage (example):
#   cmake -S Program/smallwasm -B Program/smallwasm/build-wasm -G Ninja \
#     -DLLVM_ROOT=E:/dev/llvm -DWASMOPT=E:/dev/wasm/wasm-opt.exe
#   cmake --build Program/smallwasm/build-wasm

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

set(DIST_DIR "${CMAKE_CURRENT_LIST_DIR}/web/dist" CACHE PATH "Deploy directory served by your web server (contains *.wasm)")
file(MAKE_DIRECTORY "${DIST_DIR}")
set(TMP_DIR "${CMAKE_BINARY_DIR}/tmp")
file(MAKE_DIRECTORY "${TMP_DIR}")


# -------------------------------
# wasm-opt (optional)
# -------------------------------
function(smallwasm_opt in_wasm out_wasm)
  if(WASMOPT)
    add_custom_command(
      OUTPUT "${out_wasm}"
      COMMAND "${WASMOPT}" -Oz --strip-dwarf --strip-producers --strip-names "${in_wasm}" -o "${out_wasm}"
      DEPENDS "${in_wasm}"
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT "${out_wasm}"
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${in_wasm}" "${out_wasm}"
      DEPENDS "${in_wasm}"
      VERBATIM
    )
  endif()
endfunction()

#
# Target 3: gl (context handles + C++ drives WebGL + format log)
#
set(GL_SRC "${CMAKE_CURRENT_LIST_DIR}/src/gl_ctx.cpp")
set(GL_OBJ "${CMAKE_BINARY_DIR}/gl_ctx.o")
set(RT_SRC "${CMAKE_CURRENT_LIST_DIR}/src/util/wasm_runtime.cpp")
set(RT_OBJ "${CMAKE_BINARY_DIR}/wasm_runtime.o")
set(OBJ_SRC "${CMAKE_CURRENT_LIST_DIR}/src/game/object.cpp")
set(OBJ_OBJ "${CMAKE_BINARY_DIR}/object.o")
set(CMD_SRC "${CMAKE_CURRENT_LIST_DIR}/src/graphics/wasm_command_buffer.cpp")
set(CMD_OBJ "${CMAKE_BINARY_DIR}/wasm_command_buffer.o")
set(R2D_SRC "${CMAKE_CURRENT_LIST_DIR}/src/graphics/renderer_2d.cpp")
set(R2D_OBJ "${CMAKE_BINARY_DIR}/renderer_2d.o")
set(TEX_SRC "${CMAKE_CURRENT_LIST_DIR}/src/graphics/texture_manager.cpp")
set(TEX_OBJ "${CMAKE_BINARY_DIR}/texture_manager.o")
set(UI_SRC "${CMAKE_CURRENT_LIST_DIR}/src/ui/ui_manager.cpp")
set(UI_OBJ "${CMAKE_BINARY_DIR}/ui_manager.o")
set(ENG_SRC "${CMAKE_CURRENT_LIST_DIR}/src/engine/engine.cpp")
set(ENG_OBJ "${CMAKE_BINARY_DIR}/engine.o")
set(DEMO_SRC "${CMAKE_CURRENT_LIST_DIR}/src/demo/demo_game.cpp")
set(DEMO_OBJ "${CMAKE_BINARY_DIR}/demo_game.o")
set(GL_RAW "${TMP_DIR}/output.gl.raw.wasm")
set(GL_WASM "${DIST_DIR}/output.gl.wasm")
set(GL_MAP_REPORT "${TMP_DIR}/output.gl.map.report.txt")




# Common flags (keep CMakeLists short and consistent).
set(SMALLWASM_DEBUG_DEFAULT OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(SMALLWASM_DEBUG_DEFAULT ON)
endif()
set(SMALLWASM_DEBUG ${SMALLWASM_DEBUG_DEFAULT} CACHE BOOL "Define DEBUG macro for smallwasm (enables demo/debug code)")

set(SMALLWASM_CXX_FLAGS
  --target=wasm32
  -Oz
  -ffreestanding -nostdlib
  -fno-exceptions -fno-rtti -fno-threadsafe-statics
  -std=c++23
)
if(SMALLWASM_DEBUG)
  list(APPEND SMALLWASM_CXX_FLAGS -DDEBUG=1)
endif()
set(SMALLWASM_LD_FLAGS
  --no-entry --export-memory --allow-undefined
  --export=malloc --export=free
  --export=init --export=frame --export=resize --export=pointer
  --export=on_tex_loaded --export=on_tex_failed
)
if(SMALLWASM_DEBUG)
  list(APPEND SMALLWASM_LD_FLAGS
    --export=get_context_handle
    --export=is_inited
    --export=get_frame_no
    --export=get_draw_calls
    --export=get_vertex_count
    --export=get_instance_count
    --export=get_cmd_count
    --export=wasm_heap_alloc_count
    --export=wasm_heap_free_count
    --export=wasm_heap_alloc_fail_count
    --export=wasm_heap_used_bytes
    --export=wasm_heap_free_list_bytes
    --export=wasm_heap_capacity_bytes
    --export=wasm_heap_base_addr
    --export=wasm_heap_ptr_addr
  )
endif()
string(JOIN " " SMALLWASM_LD_FLAGS_STR ${SMALLWASM_LD_FLAGS})

add_custom_command(
  OUTPUT "${GL_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${GL_SRC}" -o "${GL_OBJ}"
  DEPENDS "${GL_SRC}"
          "${CMAKE_CURRENT_LIST_DIR}/src/logfmt.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/util/wasm_compat.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/util/timer.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/util/stat.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/math/Matrix4.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/Container/SArray.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/Container/SVector.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/object.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/component.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/node.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/scene.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/render_context.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/transform.h"
          "${CMAKE_CURRENT_LIST_DIR}/src/game/sprite_renderer.h"
  VERBATIM
)

add_custom_command(
  OUTPUT "${OBJ_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${OBJ_SRC}" -o "${OBJ_OBJ}"
  DEPENDS "${OBJ_SRC}" "${CMAKE_CURRENT_LIST_DIR}/src/game/object.h"
  VERBATIM
)
add_custom_command(
  OUTPUT "${RT_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${RT_SRC}" -o "${RT_OBJ}"
  DEPENDS "${RT_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${CMD_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${CMD_SRC}" -o "${CMD_OBJ}"
  DEPENDS "${CMD_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${R2D_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${R2D_SRC}" -o "${R2D_OBJ}"
  DEPENDS "${R2D_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${TEX_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${TEX_SRC}" -o "${TEX_OBJ}"
  DEPENDS "${TEX_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${UI_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${UI_SRC}" -o "${UI_OBJ}"
  DEPENDS "${UI_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${ENG_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${ENG_SRC}" -o "${ENG_OBJ}"
  DEPENDS "${ENG_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${DEMO_OBJ}"
  COMMAND "${SMALLWASM_CLANG}" ${SMALLWASM_CXX_FLAGS} -c "${DEMO_SRC}" -o "${DEMO_OBJ}"
  DEPENDS "${DEMO_SRC}"
  VERBATIM
)

add_custom_command(
  OUTPUT "${GL_RAW}"
  COMMAND "${SMALLWASM_WASM_LD}" ${SMALLWASM_LD_FLAGS} --strip-all "${GL_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}" "${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}" -o "${GL_RAW}"
  DEPENDS "${GL_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}" "${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}"
  VERBATIM
)

smallwasm_opt("${GL_RAW}" "${GL_WASM}")

# Generate a wasm-ld link map for size analysis.
# Writes:
#   build-wasm/tmp/output.gl.map.txt
#   build-wasm/tmp/output.gl.map.raw.wasm   (link output used to produce the map; not deployed)
set(GL_MAP_TXT "${TMP_DIR}/output.gl.map.txt")
set(GL_MAP_RAW "${TMP_DIR}/output.gl.map.raw.wasm")

if(WIN32)
  add_custom_command(
    OUTPUT "${GL_MAP_TXT}" "${GL_MAP_RAW}"
    COMMAND powershell -NoProfile -Command "& '${SMALLWASM_WASM_LD}' ${SMALLWASM_LD_FLAGS_STR} '${GL_OBJ}' '${RT_OBJ}' '${OBJ_OBJ}' '${CMD_OBJ}' '${R2D_OBJ}' '${TEX_OBJ}' '${UI_OBJ}' '${ENG_OBJ}' '${DEMO_OBJ}' -o '${GL_MAP_RAW}' --print-map | Out-File -Encoding ascii '${GL_MAP_TXT}'"
    DEPENDS "${GL_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}" "${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}"
    VERBATIM
  )
else()
  add_custom_command(
    OUTPUT "${GL_MAP_TXT}" "${GL_MAP_RAW}"
    COMMAND /bin/sh -c "\"${SMALLWASM_WASM_LD}\" ${SMALLWASM_LD_FLAGS_STR} \"${GL_OBJ}\" \"${RT_OBJ}\" \"${OBJ_OBJ}\" \"${CMD_OBJ}\" \"${R2D_OBJ}\" \"${TEX_OBJ}\" \"${UI_OBJ}\" \"${ENG_OBJ}\" \"${DEMO_OBJ}\" -o \"${GL_MAP_RAW}\" --print-map > \"${GL_MAP_TXT}\""
    DEPENDS "${GL_OBJ}" "${RT_OBJ}" "${OBJ_OBJ}" "${CMD_OBJ}" "${R2D_OBJ}" "${TEX_OBJ}" "${UI_OBJ}" "${ENG_OBJ}" "${DEMO_OBJ}"
    VERBATIM
  )
endif()

add_custom_target(smallwasm_map
  DEPENDS "${GL_MAP_TXT}"
)

# Human-friendly map report.
add_custom_command(
  OUTPUT "${GL_MAP_REPORT}"
  COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
          "${CMAKE_CURRENT_LIST_DIR}/tools/map_report.ps1"
          -MapPath "${GL_MAP_TXT}"
          -OutPath "${GL_MAP_REPORT}"
          -TopN 30
  DEPENDS "${GL_MAP_TXT}" "${CMAKE_CURRENT_LIST_DIR}/tools/map_report.ps1"
  VERBATIM
)

# Default build: produce wasm + map (size analysis).
add_custom_target(smallwasm ALL
  DEPENDS "${GL_WASM}"
          "${GL_MAP_TXT}"
          "${GL_MAP_REPORT}"
)

message(STATUS "smallwasm clang   = ${SMALLWASM_CLANG}")
message(STATUS "smallwasm wasm-ld = ${SMALLWASM_WASM_LD}")
message(STATUS "smallwasm wasm-opt= ${WASMOPT}")
message(STATUS "smallwasm dist dir= ${DIST_DIR}")
