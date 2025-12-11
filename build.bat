
@echo off

IF [%1] == [] (
    set PRESET_NAME=msvc-win64-release
) ELSE (
    set PRESET_NAME=%1
)

echo Building %PRESET_NAME%

cmake --preset %PRESET_NAME%&&cmake --build --preset %PRESET_NAME%
