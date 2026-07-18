( 5axis butterfly path demo — XYZAC via PathMove )
( Program units: G21 = mm absolute; NcParser outputs meters for the controller )
( XY: butterfly outline ~±80 mm | Z=120 mm )
( A/C orientation via motion.xml PathMove rx/endRx rz/endRz )
( Soft limits (controller m): X±0.40 Y±0.30 Z 0~0.40 )
G21
G90
( --- approach --- )
G0 Z180.0
G0 X0.0000 Y54.2
( --- butterfly contour at Z=120 --- )
G1 X0.0000 Y54.2 Z120.0 F50.0
G1 X-10.3 Y43.9 Z120.0
G1 X-31.0 Y59.4 Z120.0
G1 X-58.1 Y67.1 Z120.0
G1 X-80.0 Y51.6 Z120.0
G1 X-67.1 Y25.8 Z120.0
G1 X-43.9 Y9.0 Z120.0
G1 X-71.0 Y-15.5 Z120.0
G1 X-58.1 Y-49.0 Z120.0
G1 X-31.0 Y-40.0 Z120.0
G1 X-10.3 Y-18.1 Z120.0
G1 X0.0000 Y-46.5 Z120.0
G1 X10.3 Y-18.1 Z120.0
G1 X31.0 Y-40.0 Z120.0
G1 X58.1 Y-49.0 Z120.0
G1 X71.0 Y-15.5 Z120.0
G1 X43.9 Y9.0 Z120.0
G1 X67.1 Y25.8 Z120.0
G1 X80.0 Y51.6 Z120.0
G1 X58.1 Y67.1 Z120.0
G1 X31.0 Y59.4 Z120.0
G1 X10.3 Y43.9 Z120.0
G1 X0.0000 Y54.2 Z120.0
( --- retract --- )
G0 Z180.0
G0 X0.0 Y0.0
M30
