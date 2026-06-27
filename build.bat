@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul 2>&1
cd /d C:\Users\zhang\Projects\TownMarket

if not exist build\_deps\raylib-build\raylib\Release\raylib.lib (
    echo Missing build\_deps\raylib-build\raylib\Release\raylib.lib
    echo Run the original CMake dependency setup before using this fast Release build.
    exit /b 1
)

if not exist build\TownMarket.dir\Release mkdir build\TownMarket.dir\Release
if not exist build\Release mkdir build\Release

"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe" ^
  /c /I"build\_deps\raylib-src\src" /I"build\_deps\raylib-src\src\external\glfw\include" ^
  /nologo /W4 /WX- /diagnostics:column /Od /Ob0 ^
  /D _MBCS /D WIN32 /D _WINDOWS /D NDEBUG /D PLATFORM_DESKTOP /D GRAPHICS_API_OPENGL_33 /D "CMAKE_INTDIR=\"Release\"" ^
  /Gm- /EHsc /MD /GS /fp:precise /Zc:wchar_t /Zc:forScope /Zc:inline /std:c++17 /permissive- ^
  /Fo"build\TownMarket.dir\Release\\" /Fd"build\TownMarket.dir\Release\vc143.pdb" /external:W4 /Gd /TP /errorReport:none ^
  src\main.cpp
if %ERRORLEVEL% NEQ 0 exit /b %ERRORLEVEL%

"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe" ^
  /OUT:"build\Release\TownMarket.exe" /INCREMENTAL:NO /NOLOGO ^
  "build\_deps\raylib-build\raylib\Release\raylib.lib" WINMM.LIB OPENGL32.LIB GLU32.LIB WINMM.LIB ^
  KERNEL32.LIB USER32.LIB GDI32.LIB WINSPOOL.LIB SHELL32.LIB OLE32.LIB OLEAUT32.LIB UUID.LIB COMDLG32.LIB ADVAPI32.LIB ^
  /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed ^
  /PDB:"build\Release\TownMarket.pdb" /SUBSYSTEM:CONSOLE /TLBID:1 /DYNAMICBASE /NXCOMPAT ^
  /IMPLIB:"build\Release\TownMarket.lib" /MACHINE:X64 build\TownMarket.dir\Release\main.obj
