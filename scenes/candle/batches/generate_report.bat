@set PB2D_RADIUS=0.25
@set PP3D_RADIUS=0.25
@set BB1D_RADIUS=0.25

@set ALG=upbp_pb2d
@set NAME=pb2d_mfp1.exr
call run.bat

@set ALG=upbp_bb1d
@set NAME=bb1d_mfp1.exr
call run.bat

@set ALG=upbp_pp3d
@set NAME=pp3d_mfp1.exr
call run.bat

@set PB2D_RADIUS=0.05
@set PP3D_RADIUS=0.05
@set BB1D_RADIUS=0.05

@set ALG=upbp_pb2d
@set NAME=bp2d_mfp02.exr
call run.bat

@set ALG=upbp_bb1d
@set NAME=bb1d_mfp02.exr
call run.bat

@set ALG=upbp_pp3d
@set NAME=pp3d_mfp02.exr
call run.bat

@set PB2D_RADIUS=1.25
@set PP3D_RADIUS=1.25
@set BB1D_RADIUS=1.25

@set ALG=upbp_pb2d
@set NAME=bp2d_mfp5.exr
call run.bat

@set ALG=upbp_bb1d
@set NAME=bb1d_mfp5.exr
call run.bat

@set ALG=upbp_pp3d
@set NAME=pp3d_mfp5.exr
call run.bat