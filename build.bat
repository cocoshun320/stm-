@echo off
chcp 65001 >nul
echo ============================================================
echo   STM32 安装程序 - 编译脚本
echo ============================================================
echo.

REM 查找 g++ 编译器
set "GPP="
where g++ >nul 2>nul && set "GPP=g++"
if not defined GPP if exist "C:\mingw64\mingw64\bin\g++.exe" set "GPP=C:\mingw64\mingw64\bin\g++.exe"
if not defined GPP if exist "C:\mingw64\bin\g++.exe" set "GPP=C:\mingw64\bin\g++.exe"

if not defined GPP (
    echo [错误] 未找到 g++ 编译器！
    echo.
    echo 请安装 MinGW-w64:
    echo   下载 https://github.com/brechtsanders/winlibs_mingw/releases
    echo   解压到 C:\mingw64
    echo.
    pause
    exit /b 1
)

set "WINDRES=%GPP:g++=windres%"

echo 使用编译器: %GPP%
echo.

echo [1/2] 正在编译资源文件...
"%WINDRES%" resource.rc -O coff -o resource.o
if errorlevel 1 (
    echo [错误] 资源编译失败！
    pause
    exit /b 1
)

echo [2/2] 正在编译并链接...
"%GPP%" -O2 -o "stm install.exe" installer.cpp resource.o -static
if errorlevel 1 (
    echo [错误] 编译失败！
    del resource.o 2>nul
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   编译成功！生成: stm install.exe
echo ============================================================
echo.
echo   使用方法: 双击运行 stm install.exe
echo   功能: 安装/卸载 CH340驱动 + JRE 8 + STM32CubeIDE
echo   (建议右键 -^> 以管理员身份运行)
echo.

REM 清理中间文件
del resource.o 2>nul

pause
