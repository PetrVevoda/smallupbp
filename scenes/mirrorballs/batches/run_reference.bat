@set NITER=1000000
@set ALG=upbp_all
@set PATH_LENGTH=12
@set RESOLUTION=800x800
@set PP3D_RADIUS=-0.001
@set PP3D_ALPHA=1
@set PB2D_RADIUS=-0.001
@set PB2D_ALPHA=1
@set SPM_RADIUS=-0.0015
@set SPM_ALPHA=0.75
@set BB1D_RADIUS=-0.001
@set BB1D_ALPHA=1
@set QUERY_BEAM=L
@set PHOTON_BEAM=S
@set COUT=100
@set NAME=reference.exr

"..\..\..\Build\SmallUPBP\x64\Release\SmallUPBP.exe" -s -1 "..\mirrorballs.obj" ^
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
-pbc 4000 ^
-maxMemPerThread 2000 ^
-continuous_output %COUT% ^
-o %NAME%