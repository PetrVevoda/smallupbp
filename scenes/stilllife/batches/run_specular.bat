@set NITER=1000000
@set ALG=vpts
@set PATH_LENGTH=80
@set RESOLUTION=1600x700
@set COUT=8
@set NAME=specular.exr

"..\..\..\Build\SmallUPBP\x64\Release\SmallUPBP.exe" -s -1 "..\stilllife.obj" ^
-a %ALG% ^
-l %PATH_LENGTH% ^
-i %NITER% ^
-r %RESOLUTION% ^
-seed 2657 ^
-continuous_output %COUT% ^
-o %NAME%