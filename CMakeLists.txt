cmake_minimum_required(VERSION 3.21)
project(WalkSpeedTuner VERSION 1.0.0 LANGUAGES CXX)

option(BUILD_TESTING "Build unit tests (pure-function modules only; no SKSE link)" OFF)

if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()

# ---------------------------------------------------------------------------
# Plugin DLL
# ---------------------------------------------------------------------------

if(NOT BUILD_TESTING)
    find_package(CommonLibSSE CONFIG REQUIRED)

    add_commonlibsse_plugin(${PROJECT_NAME}
        SOURCES
            src/Main.cpp
            src/MCM.cpp
            src/Settings.cpp
            src/SettingsParse.cpp
            src/WalkSpeedHook.cpp
    )

    find_package(nlohmann_json CONFIG REQUIRED)

    target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/external
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h)

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
    )

    if(DEFINED OUTPUT_FOLDER)
        set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
        message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            add_custom_command(
                TARGET "${PROJECT_NAME}"
                POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
                VERBATIM
            )
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests (pure-function modules; no SKSE / no actor / no engine)
# ---------------------------------------------------------------------------

if(BUILD_TESTING)
    find_package(Catch2 3 CONFIG REQUIRED)
    find_package(nlohmann_json CONFIG REQUIRED)

    add_executable(WalkSpeedTuner_tests
        tests/test_compute.cpp
        tests/test_throttle.cpp
        tests/test_settings.cpp
        src/SettingsParse.cpp
    )

    target_compile_features(WalkSpeedTuner_tests PRIVATE cxx_std_23)
    target_include_directories(WalkSpeedTuner_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(WalkSpeedTuner_tests PRIVATE
        Catch2::Catch2WithMain
        nlohmann_json::nlohmann_json
    )

    include(CTest)
    include(Catch)
    catch_discover_tests(WalkSpeedTuner_tests)
endif()
