@set PATH_LENGTH=80
@set RESOLUTION=1600x700
@set PP3D_RADIUS=-0.0001
@set PP3D_ALPHA=1
@set PB2D_RADIUS=-0.0001
@set PB2D_ALPHA=1
@set SPM_RADIUS=-0.0001
@set SPM_ALPHA=0.75
@set BB1D_RADIUS=-0.0001
@set BB1D_ALPHA=1
@set QUERY_BEAM=L
@set PHOTON_BEAM=S
@set IGNORE_SPEC=1

"..\..\..\Build\SmallUPBP\x64\Release\SmallUPBP.exe" -s -1 "..\stilllife.obj" ^
-a %ALG% ^
-l %PATH_LENGTH% ^
-t %NTIME% ^
-r %RESOLUTION% ^
-r_initial_pp3d %PP3D_RADIUS% ^
-r_alpha_pp3d %PP3D_ALPHA% ^
-r_initial_pb2d %PB2D_RADIUS% ^
-r_alpha_pb2d %PB2D_ALPHA% ^
-r_initial_surf %SPM_RADIUS% ^
-r_alpha_surf %SPM_ALPHA% ^
-r_initial_bb1d %BB1D_RADIUS% ^
-r_alpha_bb1d %BB1D_ALPHA% ^
-qbt %QUERY_BEAM% ^
-pbt %PHOTON_BEAM% ^
-pbc 23000 ^
-seed 2657 ^
%MODE% ^
%PREVBB1D% ^
-ignorespec %IGNORE_SPEC% ^
-o %NAME%