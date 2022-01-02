
:: Necessary Qt dlls are packaged with every release.
:: These dlls are not included in the SVN.
:: They need to be copied into the dev area from the Qt install.
:: Qt-Framework is simply the Qt runtime dlls built against the MSVC 2008 compiler
:: It can be found at: http://qt.nokia.com/downloads
:: If you build SimC with MSVC 2008, then you need to use dlls from Qt-Framework
:: As of this writing, the default locations from which to gather the dlls are:
:: Qt-Framework: C:\Qt\Qt5.3.1\

:: Update the qt_dir as necessary
set qt_dir=C:\Qt\5.9.9\msvc2017_64
set install=simc-win64-release
set redist="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\redist\x64\Microsoft.VC140.CRT"

:: IMPORTANT NOTE FOR DEBUGGING
:: This script will ONLY copy the optimized Qt dlls
:: The MSVC 2008 simcqt.vcproj file is setup to use optimized dlls for both Debug/Release builds
:: This script needs to be smarter if you wish to use the interactive debugger in the Qt SDK
:: The debug Qt dlls are named: Qt___d4.dll

:: Delete old folder/files

rd %install% /s /q
:: Copying new dlls

xcopy Simulationcraft.exe %install%\


:: Run windeployqt tool to create dependencies
%qt_dir%\bin\windeployqt.exe %install%\Simulationcraft.exe

:: Copy other relevant files for windows release
xcopy Welcome.html %install%\
xcopy Welcome.png %install%\
xcopy simc64.exe %install%\
xcopy READ_ME_FIRST.txt %install%\
xcopy Examples.simc %install%\
xcopy Error.html %install%\
xcopy COPYING %install%\
xcopy Profiles %install%\profiles\ /s /e
xcopy C:\OpenSSL-Win64\bin\libeay32.dll %install%\
xcopy C:\OpenSSL-Win64\bin\ssleay32.dll %install%\