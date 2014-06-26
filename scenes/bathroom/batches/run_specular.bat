@set NITER=1000000
@set ALG=vpts
@set PATH_LENGTH=20
@set RESOLUTION=672x896
@set COUT=100
@set NAME=specular.exr

"..\..\..\Build\SmallUPBP\x64\Release\SmallUPBP.exe" -s -1 "..\bathroom.obj" ^
-a %ALG% ^
-l %PATH_LENGTH% ^
-i %NITER% ^
-r %RESOLUTION% ^
-continuous_output %COUT% ^
-o %NAME%