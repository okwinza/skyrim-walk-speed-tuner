# Build the unit-test executable. No SKSE/CommonLibSSE-NG link — tests
# only exercise pure-function modules.

find_package(Catch2 3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(WalkSpeedTuner_tests
    tests/test_compute.cpp
    tests/test_throttle.cpp
    tests/test_settings.cpp
    tests/test_bump.cpp
    tests/test_match.cpp
    tests/test_indicator.cpp
    src/SettingsParse.cpp
)

target_compile_features(WalkSpeedTuner_tests PRIVATE cxx_std_23)
target_include_directories(WalkSpeedTuner_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(WalkSpeedTuner_tests PRIVATE
    Catch2::Catch2WithMain
    nlohmann_json::nlohmann_json
)

include(CTest)
include(Catch)
catch_discover_tests(WalkSpeedTuner_tests)
