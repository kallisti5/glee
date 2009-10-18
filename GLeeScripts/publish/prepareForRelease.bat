cd ..\..\DATA
del /Q RELEASE\*.*
cd output
zip -9 ..\RELEASE\GLee5_4.zip *.*
cd..
copy output\*.txt .\RELEASE
copy outputLinux\*.tar.gz .\RELEASE
pause