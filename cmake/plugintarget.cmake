# Build the plugin DLL: pull in CommonLibSSE-NG via the submodule, set
# its build options, link our SKSE plugin against it.

# Disable CommonLibSSE-NG's own test suite — we don't want a second
# Catch2 build inside our plugin tree. Defaults to ON in upstream
# extern/CommonLibSSE-NG/CMakeLists.txt:10, must override BEFORE
# add_subdirectory.
set(BUILD_TESTS         OFF CACHE BOOL "Disable CommonLibSSE-NG tests")

# Skyrim runtime targets. Default upstream is ON for SE/AE/VR; we ship
# AE/SE only. Disabling VR avoids pulling OpenVR headers.
set(ENABLE_SKYRIM_SE    ON  CACHE BOOL "Build for SE 1.5.x")
set(ENABLE_SKYRIM_AE    ON  CACHE BOOL "Build for AE 1.6.x")
set(ENABLE_SKYRIM_VR    OFF CACHE BOOL "Build for VR")

# Pull in CommonLibSSE-NG. Compiles as STATIC, gets bundled into our DLL.
add_subdirectory("${CMAKE_SOURCE_DIR}/extern/CommonLibSSE-NG"
                 CommonLibSSE-NG
                 EXCLUDE_FROM_ALL)

# nlohmann_json — we link it directly (settings parser).
# spdlog/DirectXTK come transitively through CommonLibSSE::CommonLibSSE.
find_package(nlohmann_json CONFIG REQUIRED)

# The plugin DLL.
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/external"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    CommonLibSSE::CommonLibSSE
    nlohmann_json::nlohmann_json
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/PCH.h"
)

# Optional auto-deploy: when SKYRIM_FOLDER or SKYRIM_MODS_FOLDER is set
# at configure time, copy the DLL to that location post-build.
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(_DEPLOY_BASE "$ENV{SKYRIM_FOLDER}/Data")
endif()
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(_DEPLOY_BASE "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()
if(DEFINED _DEPLOY_BASE)
    set(_DEPLOY_DIR "${_DEPLOY_BASE}/SKSE/Plugins")
    message(STATUS "SKSE plugin auto-deploy folder: ${_DEPLOY_DIR}")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_DEPLOY_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${_DEPLOY_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${_DEPLOY_DIR}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM)
    endif()
endif()
