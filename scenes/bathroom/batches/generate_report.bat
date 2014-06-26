@set NTIME=3600
@set MODE=-compatible
@set PREVBB1D=-previous_bb1d
@set ALG=upbp_bb1d+pb2d+pp3d+bpt
@set NAME=compatible_all_prevbb1d.exr
call run_temp.bat

@set PREVBB1D=
@set NAME=compatible_all.exr
call run_temp.bat

@set DEBUGIMG_OPTION=none
@set ALG=upbp_bb1d
@set NAME=compatible_bb1d.exr
call run_temp.bat

@set ALG=upbp_pb2d
@set NAME=compatible_pb2d.exr
call run_temp.bat

@set ALG=upbp_pp3d
@set NAME=compatible_pp3d.exr
call run_temp.bat

@set ALG=upbp_bpt
@set NAME=compatible_bpt.exr
call run_temp.bat

@set MODE=-previous
@set ALG=upbp_bb1d
@set NAME=previous_bb1d.exr
call run_temp.bat

@set ALG=upbp_pb2d
@set NAME=previous_pb2d.exr
call run_temp.bat

@set ALG=upbp_pp3d
@set NAME=previous_pp3d.exr
call run_temp.bat

@set ALG=upbp_bpt
@set NAME=previous_bpt.exr
call run_temp.bat

@set MODE=
@set ALG=upbp_all
@set NAME=complete_our.exr
call run_temp.bat

@set ALG=upbp_bpt
@set NAME=complete_bpt.exr
call run_temp.bat