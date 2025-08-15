@echo off
echo Creating Spectrum Seekbar V10 fb2k-component package...
echo =========================================================
cd /d "%~dp0"

set COMPONENT_NAME=foo_spectrum_seekbar_v10
set VERSION=10.0.0

if not exist %COMPONENT_NAME%.dll (
    echo ERROR: %COMPONENT_NAME%.dll not found!
    echo Please build the component first.
    pause
    exit /b 1
)

echo.
echo Preparing component files...

:: Create temporary directory
if exist temp_component rmdir /s /q temp_component
mkdir temp_component

:: Copy DLL
copy %COMPONENT_NAME%.dll temp_component\

:: Create info.txt
echo Creating info.txt...
(
echo Component: Spectrum Seekbar V10
echo Version: %VERSION%
echo Author: foobar2000 user
echo Description: 4 visualization styles with mono/stereo modes
echo.
echo FEATURES:
echo - 4 Visualization Styles: Lines, Bars, Blocks, Dots
echo - 2 Channel Modes: Mixed (Mono) and Stereo (Mirrored)
echo - RIGHT-CLICK opens menu to select style and channel mode
echo - Click anywhere to seek
echo - Visual progress indicators
echo - Time display and mode indicators
echo.
echo VISUALIZATION STYLES:
echo 1. Lines - Smooth connected line
echo 2. Bars - Classic vertical bars
echo 3. Blocks - Stacked block segments
echo 4. Dots - Point-based display
echo.
echo CHANNEL MODES:
echo 1. Mixed (Mono) - Single combined visualization
echo 2. Stereo (Mirrored) - Top/bottom mirrored channels
echo.
echo HOW TO USE:
echo 1. Add "Spectrum Seekbar V10" to your layout
echo 2. Play a track - default visualization appears (Bars + Mono)
echo 3. RIGHT-CLICK to open menu and select:
echo    - Visualization Style: Lines/Bars/Blocks/Dots
echo    - Channel Mode: Mixed/Stereo
echo 4. LEFT-CLICK anywhere to seek
echo.
echo Current settings are shown in top-right corner.
echo This is the complete version with all 4 styles as requested,
echo each available in both mono and stereo modes with menu selection.
) > temp_component\info.txt

:: Create the fb2k-component
echo.
echo Creating fb2k-component package...

:: Delete old package if exists
if exist %COMPONENT_NAME%-%VERSION%.fb2k-component del %COMPONENT_NAME%-%VERSION%.fb2k-component

:: Use PowerShell to create ZIP
powershell -Command "Compress-Archive -Path 'temp_component\*' -DestinationPath 'temp_component.zip' -Force"

:: Rename to fb2k-component
if exist temp_component.zip (
    move temp_component.zip %COMPONENT_NAME%-%VERSION%.fb2k-component
    echo.
    echo ========================================
    echo COMPONENT PACKAGE CREATED SUCCESSFULLY!
    echo ========================================
    echo.
    echo Package: %COMPONENT_NAME%-%VERSION%.fb2k-component
    echo.
    echo TO INSTALL:
    echo 1. FIRST uninstall any previous version
    echo 2. Double-click the .fb2k-component file
    echo 3. Restart foobar2000
    echo 4. Add "Spectrum Seekbar V10" to your layout
    echo 5. Right-click to access the full menu system!
    echo.
    echo MENU OPTIONS:
    echo - Visualization Style: Lines/Bars/Blocks/Dots
    echo - Channel Mode: Mixed (Mono)/Stereo (Mirrored)
    echo.
) else (
    echo ERROR: Failed to create component package!
)

:: Clean up
rmdir /s /q temp_component

pause