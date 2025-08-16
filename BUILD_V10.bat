@echo off
echo Building Spectrum Seekbar V10...
cd /d "%~dp0"

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64

set SDK_PATH=backup\foo_seekbar\SDK-2025-03-07

cl /c /EHsc /MD /O2 /std:c++17 /DNDEBUG /DUNICODE /D_UNICODE /DFOOBAR2000_TARGET_VERSION=82 ^
    /I"%SDK_PATH%" /I"%SDK_PATH%\foobar2000" /I"%SDK_PATH%\foobar2000\SDK" /I"%SDK_PATH%\pfc" ^
    spectrum_seekbar_v10.cpp

link /DLL /OUT:foo_spectrum_seekbar_v10.dll ^
    spectrum_seekbar_v10.obj ^
    "%SDK_PATH%\foobar2000\foobar2000_component_client\x64\Release\foobar2000_component_client.lib" ^
    "%SDK_PATH%\foobar2000\SDK\x64\Release\foobar2000_SDK.lib" ^
    "%SDK_PATH%\pfc\x64\Release\pfc.lib" ^
    "%SDK_PATH%\foobar2000\shared\shared-x64.lib" ^
    kernel32.lib user32.lib gdi32.lib shell32.lib ^
    /NODEFAULTLIB:LIBCMT

if exist foo_spectrum_seekbar_v10.dll (
    echo SUCCESS! V10 component built successfully
    del *.obj 2>nul
) else (
    echo BUILD FAILED!
)
pause