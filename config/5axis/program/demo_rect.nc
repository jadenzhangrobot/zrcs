( 5axis simultaneous demo — XYZ path + A/C via PathMove orient ports )
( Program units: G21 = mm absolute; NcParser outputs meters for the controller )
( XYZ: circle R=60 mm @ Z=100 then spiral in | A:0→0.5  C:0→1.57 via motion.xml )
( NOTE: NcParse reads XYZ only; A/C driven by PathMove rx/endRx rz/endRz )
G21
G90
( --- approach --- )
G0 Z160.0
G0 X60.0 Y0.0
( --- circle lap 1 at Z=100 --- )
G1 X60.0 Y0.0 Z100.0 F40.0
G1 X55.6 Y22.6 Z100.0
G1 X42.4 Y42.4 Z100.0
G1 X22.6 Y55.6 Z100.0
G1 X0.0 Y60.0 Z100.0
G1 X-22.6 Y55.6 Z100.0
G1 X-42.4 Y42.4 Z100.0
G1 X-55.6 Y22.6 Z100.0
G1 X-60.0 Y0.0 Z100.0
G1 X-55.6 Y-22.6 Z100.0
G1 X-42.4 Y-42.4 Z100.0
G1 X-22.6 Y-55.6 Z100.0
G1 X0.0 Y-60.0 Z100.0
G1 X22.6 Y-55.6 Z100.0
G1 X42.4 Y-42.4 Z100.0
G1 X55.6 Y-22.6 Z100.0
G1 X60.0 Y0.0 Z100.0
( --- circle lap 2 at Z=110 --- )
G1 X55.6 Y22.6 Z110.0
G1 X42.4 Y42.4 Z110.0
G1 X22.6 Y55.6 Z110.0
G1 X0.0 Y60.0 Z110.0
G1 X-22.6 Y55.6 Z110.0
G1 X-42.4 Y42.4 Z110.0
G1 X-55.6 Y22.6 Z110.0
G1 X-60.0 Y0.0 Z110.0
G1 X-55.6 Y-22.6 Z110.0
G1 X-42.4 Y-42.4 Z110.0
G1 X-22.6 Y-55.6 Z110.0
G1 X0.0 Y-60.0 Z110.0
G1 X22.6 Y-55.6 Z110.0
G1 X42.4 Y-42.4 Z110.0
G1 X55.6 Y-22.6 Z110.0
G1 X60.0 Y0.0 Z110.0
( --- spiral in to center --- )
G1 X40.0 Y20.0 Z120.0
G1 X20.0 Y30.0 Z125.0
G1 X0.0 Y20.0 Z130.0
G1 X-10.0 Y0.0 Z132.0
G1 X0.0 Y0.0 Z135.0
( --- retract --- )
G0 Z160.0
G0 X0.0 Y0.0
M30
