# dsh-plugin-zrcs-mujoco

This DeepSeek Harness plugin accepts one natural-language robot task. The
model is instructed to run a visual closed loop: observe the MuJoCo scene,
issue one safe motion, wait for it to settle, observe again, and continue until
the goal is visibly complete or the task must stop.

It exposes `get_mujoco_frame`, `get_robot_status`, `move_abs_j`, `move_j`, and
`wait_robot_motion`. Motion requests are validated by the ZRCS GUI and
forwarded through the existing ZMQ/Protobuf control path.

The tool requests a screenshot only when the model calls it. The ZRCS GUI
serves the current `QOpenGLWidget` framebuffer at:

```text
http://127.0.0.1:8765/mujoco/latest.jpg
```

Motion endpoint:

```text
POST http://127.0.0.1:8765/robot/motion
```

Robot status endpoint:

```text
GET http://127.0.0.1:8765/robot/status
```

In the Harness chat, use the tools with prompts such as:

```text
调用 move_abs_j，把各关节移动到 [0, -0.4, 0.8, 0, 0, 0] 弧度，然后调用 get_mujoco_frame 检查结果。
```

```text
调用 move_j 移动到 x=0.30, y=0.00, z=0.25, rx=0, ry=1.57, rz=0，速度比例 0.5，然后获取 MuJoCo 图像。
```

`move_abs_j` 使用弧度；`move_j` 使用米和弧度。工具返回 `accepted` 代表
ZRCS 已接收命令，动作完成后的视觉确认应单独调用 `get_mujoco_frame`。

The endpoint is local-only and does not keep a background image stream.

Install the plugin into the Harness web profile with the Harness launcher:

```powershell
npx.cmd --yes @deepseek-ai/dsh plugin --profile web add "E:\zrcs-dev\tools\dsh-plugin-zrcs-mujoco"
```

Then add the plugin to `C:\Users\<you>\.dsh\profiles\web\cordis.patch.yml`:

```yaml
- insert:
    - id: zrcs-mujoco
      name: dsh-plugin-zrcs-mujoco
```

The GUI listens on port `8765` by default. To avoid a port conflict, set
`ZRCS_MUJOCO_PORT` before starting ZRCS, then set `ZRCS_MUJOCO_URL` to the
same port in the Harness process.

Set `ZRCS_MUJOCO_URL` only when the GUI endpoint uses a different host or port.
The Harness profile must load `@deepseek-ai/dsh-attachment-local` so the JPEG
can be returned as a durable image attachment.
