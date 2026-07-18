( demo_rect.nc — simple rectangle for path pipeline bring-up )
( Program units: G21 = mm absolute; NcParser outputs meters for the controller )
G21
G90
G0 Z5.0
G0 X0.0 Y0.0
G1 Z-1.0 F200.0
G1 X50.0 Y0.0 F800.0
G1 X50.0 Y40.0
G1 X0.0 Y40.0
G1 X0.0 Y0.0
G0 Z5.0
M30
