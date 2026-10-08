set "PATH=%QTDIR%\bin;%PATH%"

rem folder which holds the deps fetched by a previous run (offline build)
rem it holds the CefViewCore source tree with the CEF binary SDK in its dep folder,
rem so that removing the build folder will not download them again
set "OFFLINE_DEPS_DIR=%cd%\deps"

if exist ".build\windows.x86_64" rmdir /s /q ".build\windows.x86_64"

rem "Visual Studio 16 2019"
rem "Visual Studio 17 2022"

rem deploy the CEF runtime files (libcef.dll, CefViewWing.exe, locales, ...)
rem directly next to the application binaries instead of into a CefView subfolder,
rem append -DCEF_RUNTIME_IN_SUBDIR=ON to the command line to get the subfolder back
cmake -G "Visual Studio 17 2022" ^
-S . ^
-B .build/windows.x86_64 ^
-A x64 ^
-T v142 ^
-DPROJECT_ARCH=x86_64 ^
-DCEFVIEW_WING_NAME=TBBrowserProcess ^
-DBUILD_DEMO=ON ^
-DCEF_RUNTIME_IN_SUBDIR=OFF ^
-DQCEFVIEW_OFFLINE_DEPS_DIR="%OFFLINE_DEPS_DIR%" ^
-DCMAKE_INSTALL_PREFIX:PATH="%cd%/out/windows.x86_64" %*
