---
name: zrcs-robot-visual-control
description: Complete ZRCS MuJoCo robot tasks using only live scene images, robot status, and motion tools; never inspect files or configuration.
---

# ZRCS Visual Robot Control

Use this skill for natural-language tasks that ask the ZRCS robot to manipulate or reach something in the MuJoCo scene.

## Hard boundaries

- The only task tools allowed are `get_mujoco_frame`, `get_robot_status`, `wait_robot_motion`, `move_j`, and `move_abs_j`.
- Do not call any file, directory, shell, command, search, or source-inspection tool. This includes `read`, `glob`, `grep`, `bash`, PowerShell, `exec`, `git`, and equivalent tools.
- Never open or search configuration files, XML files, source files, model files, `axis.xml`, `robot.xml`, `mujoco.xml`, or any project directory.
- Do not infer object locations, robot geometry, joint limits, or current state from files. The live `get_mujoco_frame` image and live robot status are the only sources of scene and state information.
- Do not load another skill for this robot task. Do not report coordinates or completion that were not provided by the user or supported by the current image and status.

## Closed-loop procedure

1. Capture `get_mujoco_frame` before the first motion. Use `get_robot_status` when availability, motion, or servo state matters.
2. Choose one conservative next action. Use `move_abs_j` only for a complete absolute joint target. Use `move_j` only when a safe Cartesian pose is known. Default `velocity_scale` to `0.5` or lower.
3. Issue no more than one motion command before waiting. Treat an accepted response as acknowledgement, not completion.
4. Pass the returned `motionId` to `wait_robot_motion`. If it cannot settle, stop.
5. Capture `get_mujoco_frame` after settling and check the actual visual result. Continue one action at a time only when the image shows the task is not complete and the next action is safe.
6. Stop when the image is unclear, the robot is unavailable, a collision or limit risk is visible, the command is rejected, or repeated actions make no visible progress.
7. Claim success only when the final live image visibly confirms the requested result.
