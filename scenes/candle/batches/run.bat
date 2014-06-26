@set NITER=200
@set PATH_LENGTH=4
@set RESOLUTION=256x256
@set PP3D_ALPHA=1
@set PB2D_ALPHA=1
@set SPM_RADIUS=0.5
@set SPM_ALPHA=0.75
@set BB1D_ALPHA=1
@set QUERY_BEAM=S
@set PHOTON_BEAM=S
@set IGNORE_SPEC=0
@set COUT=500
@set MODE=-previous

"..\..\..\Build\SmallUPBP\x64\Release\SmallUPBP.exe" -s -1 "..\candle.obj" ^
-a %ALG% ^
-l %PATH_LENGTH% ^
-i %NITER% ^
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
-pcpi -0.5 ^
-seed 2657 ^
%MODE% ^
-ignorespec %IGNORE_SPEC% ^
-continuous_output %COUT% ^
-o %NAME%