# Collect plugin sources + generate the SKSEPlugin_Version declaration.

set(SOURCE_FILES
    src/Hotkey.cpp
    src/InputSuppress.cpp
    src/Main.cpp
    src/MCM.cpp
    src/Settings.cpp
    src/SettingsParse.cpp
    src/WalkSpeedHook.cpp
)

# Generate plugin.cpp from the template, substituting ${PROJECT_VERSION_*}
# into the SKSEPluginInfo macro. Keeps version coupled to project() only.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/plugin.cpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/plugin.cpp"
    @ONLY)

list(APPEND SOURCE_FILES "${CMAKE_CURRENT_BINARY_DIR}/plugin.cpp")
