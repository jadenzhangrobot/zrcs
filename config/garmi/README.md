# GARMI configuration

This project uses the GARMI MuJoCo model from
`tenfoldpaper/multipanda_ros2/garmi_packages/garmi_description/mujoco/garmi`.
The model contains a mobile base, head, and two Panda arms. ZRCS binds the
two seven-axis arms, both grippers, the two drive wheels, and the two head
axes as 20 logical axes. The MoveJ/MoveL Cartesian model entries target the
left and right seven-axis arms; the remaining axes are available through
single-axis commands.

`mujoco.xml` enables `kinematicOnly` because ZRCS owns the position loop and
the upstream model declares several alternative MuJoCo actuators per joint.
This keeps the ZRCS position command mapped deterministically to the imported
joint state while retaining the original meshes and joint limits.

The upstream repository is licensed under Apache License 2.0. Its source
license and notices are available at:
https://github.com/tenfoldpaper/multipanda_ros2
