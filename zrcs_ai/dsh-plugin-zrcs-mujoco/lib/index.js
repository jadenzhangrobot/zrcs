import { defineTool } from "@deepseek-ai/dsh-tools";

export const name = "zrcs-mujoco";
export const inject = ["tools", "attachments", "systemPrompt", "skills"];

const DEFAULT_BASE_URL = "http://127.0.0.1:8765";
const DEFAULT_TIMEOUT_MS = 5000;
const DEFAULT_POLL_MS = 100;
const DEFAULT_SETTLE_MS = 300;
const DEFAULT_START_GRACE_MS = 500;

const ROBOT_SKILL_NAME = "zrcs-robot-visual-control";

const ROBOT_SKILL_DESCRIPTION =
  "Complete ZRCS MuJoCo robot tasks using only the live MuJoCo image, robot status, and motion tools; never inspect files or configuration.";

const ROBOT_SKILL_CONTENT = `
# ZRCS Visual Robot Control

Use this skill for every task that asks the ZRCS robot to manipulate or reach something in the MuJoCo scene.

## Hard boundaries

- The only task tools allowed are get_mujoco_frame, get_robot_status, wait_robot_motion, move_j, and move_abs_j.
- Do not call any file, directory, shell, command, search, or source-inspection tool. This includes read, glob, grep, bash, PowerShell, exec, git, and equivalent tools.
- Never open or search config files, XML files, source files, model files, axis.xml, robot.xml, mujoco.xml, or any project directory.
- Do not infer object locations, robot geometry, joint limits, or current state from files. The live get_mujoco_frame image and the live robot status are the only sources of scene and state information.
- Do not load another skill for this robot task. Do not report coordinates or completion that were not provided by the user or supported by the current image and status.

## Closed-loop procedure

1. Capture get_mujoco_frame before the first motion. Use get_robot_status when availability, motion, or servo state matters.
2. Choose one conservative next action. Use move_abs_j only for a complete absolute joint target. Use move_j only when a safe Cartesian pose is known. Default velocity_scale to 0.5 or lower.
3. Issue no more than one motion command before waiting. Treat an accepted response as acknowledgement, not completion.
4. Pass the returned motionId to wait_robot_motion. If it cannot settle, stop.
5. Capture get_mujoco_frame after settling and check the actual visual result. Continue one action at a time only when the image shows the task is not complete and the next action is safe.
6. Stop when the image is unclear, the robot is unavailable, a collision or limit risk is visible, the command is rejected, or repeated actions make no visible progress.
7. Claim success only when the final live image visibly confirms the requested result.
`;

const TASK_AGENT_INSTRUCTIONS = `
You are controlling a ZRCS robot through the available MuJoCo and motion tools.
This is a visual-control task. The live MuJoCo image and live robot status are the only permitted sources of truth.
Hard restrictions:
- Use only get_mujoco_frame, get_robot_status, wait_robot_motion, move_j, and move_abs_j for the robot task.
- Never call file, directory, shell, command, search, or source-inspection tools, including read, glob, grep, bash, PowerShell, exec, git, or equivalents.
- Never read or search config files, XML, source code, model files, axis.xml, robot.xml, mujoco.xml, or any project directory.
- Do not infer scene geometry, object coordinates, joint limits, or robot state from files. If the live image or status cannot establish a safe next step, stop and explain what is missing.
When the user gives a robot task, execute it as a closed-loop visual task instead of asking the user to call tools manually:
1. Call get_mujoco_frame before the first motion and inspect the current robot, workpiece, obstacles, and target.
2. Plan only the next safe motion from the observed state. Use move_abs_j for known joint targets or move_j for a Cartesian pose. Use conservative velocity_scale values (normally 0.5 or lower) unless the user explicitly requires otherwise.
3. Execute at most one motion command at a time. Treat the command response as acceptance only, not as proof that the motion finished.
4. After every accepted motion, call wait_robot_motion with the returned motionId, then call get_mujoco_frame again.
5. Compare the new image with the goal. If the goal is not visibly complete, re-plan one next motion and repeat the observe -> act -> wait -> observe cycle.
6. Stop and explain the reason if the robot status is unavailable, the image is unclear, a limit/collision risk is visible, the motion is rejected, or repeated actions make no progress.
7. Do not claim success until the final image visibly confirms the requested result. Do not invent coordinates or pretend an action completed.
`;

function configuredBaseUrl() {
  const value = process.env.ZRCS_MUJOCO_URL || DEFAULT_BASE_URL;
  return value.replace(/\/+$/, "");
}

function mediaTypeFromHeader(value) {
  const mediaType = (value || "image/jpeg").split(";", 1)[0].trim().toLowerCase();
  return ["image/png", "image/jpeg", "image/webp", "image/gif"].includes(mediaType)
    ? mediaType
    : "image/jpeg";
}

function timeoutValue(value, fallback = DEFAULT_TIMEOUT_MS, maximum = 30000) {
  const requested = Number(value);
  return Number.isFinite(requested) && requested > 0
    ? Math.min(requested, maximum)
    : fallback;
}

async function readError(response) {
  const text = await response.text().catch(() => "");
  if (!text) {
    return `${response.status} ${response.statusText}`.trim();
  }
  try {
    const parsed = JSON.parse(text);
    return parsed.error || text;
  } catch {
    return text;
  }
}

async function fetchJson(path, { method = "GET", body, signal, timeoutMs = DEFAULT_TIMEOUT_MS } = {}) {
  const controller = new AbortController();
  const abortFromCaller = () => controller.abort();
  signal?.addEventListener?.("abort", abortFromCaller, { once: true });
  const timer = setTimeout(() => controller.abort(), timeoutMs);
  let response;
  try {
    response = await fetch(`${configuredBaseUrl()}${path}`, {
      method,
      headers: body === undefined ? undefined : { "content-type": "application/json" },
      cache: "no-store",
      body: body === undefined ? undefined : JSON.stringify(body),
      signal: controller.signal,
    });
  } catch (error) {
    if (error?.name === "AbortError") {
      throw new Error(`ZRCS request ${path} timed out after ${timeoutMs} ms.`);
    }
    throw new Error(`ZRCS request ${path} failed: ${error?.message || error}`);
  } finally {
    clearTimeout(timer);
    signal?.removeEventListener?.("abort", abortFromCaller);
  }

  const text = await response.text();
  let value;
  try {
    value = text ? JSON.parse(text) : {};
  } catch {
    value = { error: text || `${response.status} ${response.statusText}` };
  }
  if (!response.ok) {
    throw new Error(`ZRCS request ${path} failed: ${value.error || value.reply || response.statusText}`);
  }
  return value;
}

async function postMotion(command, args, signal) {
  return fetchJson("/robot/motion", {
    method: "POST",
    body: { command, args },
    signal,
    timeoutMs: 15000,
  });
}

function sleep(ms, signal) {
  return new Promise((resolve, reject) => {
    let settled = false;
    const timer = setTimeout(() => {
      settled = true;
      signal?.removeEventListener?.("abort", abort);
      resolve();
    }, ms);
    const abort = () => {
      if (settled) return;
      settled = true;
      clearTimeout(timer);
      signal?.removeEventListener?.("abort", abort);
      reject(new Error("robot motion wait was cancelled"));
    };
    signal?.addEventListener?.("abort", abort, { once: true });
  });
}

function assertFiniteNumbers(values, label) {
  if (!Array.isArray(values) || values.length === 0 || values.some((value) => !Number.isFinite(Number(value)))) {
    throw new Error(`${label} must be a non-empty array of finite numbers.`);
  }
  return values.map(Number);
}

function motionOutput() {
  return {
    schema: {
      type: "object",
      additionalProperties: false,
      properties: {
        ok: { type: "boolean", required: true },
        command: { type: "string", required: true },
        args: {
          type: "array",
          items: { type: "number" },
          required: true,
        },
        motionId: { type: "number", required: true },
        reply: { type: "string", required: true },
      },
    },
    render: (_args, value) => [
      {
        type: "text",
        text: `${value.command}: ${value.ok ? "accepted" : "rejected"}` +
          `${value.ok ? ` (motionId=${value.motionId})` : ""}. ${value.reply || ""}`.trim(),
      },
    ],
  };
}

export function apply(ctx) {
  if (ctx.skills && typeof ctx.skills.register === "function") {
    ctx.skills.register({
      name: ROBOT_SKILL_NAME,
      description: ROBOT_SKILL_DESCRIPTION,
      whenToUse: "Use for natural-language ZRCS robot tasks that require MuJoCo visual feedback and robot motion.",
      source: "bundled",
      content: ROBOT_SKILL_CONTENT,
    });
  } else {
    ctx.logger?.warn?.("dsh-plugin-zrcs-mujoco: skills service is unavailable; robot skill was not registered");
  }

  if (ctx.systemPrompt && typeof ctx.systemPrompt.section === "function") {
    ctx.systemPrompt.section({
      name: "zrcs-mujoco:task-agent",
      order: 90,
      text: TASK_AGENT_INSTRUCTIONS,
    });
  } else {
    ctx.logger?.warn?.("dsh-plugin-zrcs-mujoco: systemPrompt service is unavailable; task loop guidance was not installed");
  }

  ctx.tools.register(
    defineTool({
      name: "get_mujoco_frame",
      description:
        "Capture the current ZRCS MuJoCo GUI frame on demand and return it as an image. " +
        "Use this after a robot action when visual feedback is needed; it does not stream frames.",
      parameters: {
        timeout_ms: {
          type: "number",
          description: "HTTP timeout in milliseconds. Defaults to 5000.",
        },
      },
      output: {
        schema: {
          type: "object",
          additionalProperties: false,
          properties: {
            image: {
              type: "object",
              additionalProperties: true,
              required: true,
            },
            capturedAt: {
              type: "string",
              required: true,
            },
            width: {
              type: "number",
              required: true,
            },
            height: {
              type: "number",
              required: true,
            },
          },
        },
        render: (_args, value) => [
          {
            type: "text",
            text: `MuJoCo frame captured at ${value.capturedAt} (${value.width}x${value.height}).`,
          },
          {
            type: "image",
            attachment: value.image,
          },
        ],
      },
      async execute(args, exec) {
        const attachments = ctx.attachments || ctx.get?.("attachments");
        if (!attachments || typeof attachments.saveImage !== "function") {
          throw new Error("Harness attachment service is unavailable; enable dsh-attachment-local.");
        }

        const requestedTimeout = Number(args?.timeout_ms);
        const timeoutMs = Number.isFinite(requestedTimeout) && requestedTimeout > 0
          ? Math.min(requestedTimeout, 30000)
          : DEFAULT_TIMEOUT_MS;
        const controller = new AbortController();
        const abortFromHarness = () => controller.abort();
        exec?.signal?.addEventListener?.("abort", abortFromHarness, { once: true });
        const timer = setTimeout(() => controller.abort(), timeoutMs);

        let response;
        try {
          response = await fetch(`${configuredBaseUrl()}/mujoco/latest.jpg`, {
            method: "GET",
            cache: "no-store",
            signal: controller.signal,
          });
        } catch (error) {
          if (error?.name === "AbortError") {
            throw new Error(`MuJoCo screenshot request timed out after ${timeoutMs} ms.`);
          }
          throw new Error(`MuJoCo screenshot request failed: ${error?.message || error}`);
        } finally {
          clearTimeout(timer);
          exec?.signal?.removeEventListener?.("abort", abortFromHarness);
        }

        if (!response.ok) {
          throw new Error(`MuJoCo screenshot endpoint failed: ${await readError(response)}`);
        }

        const data = new Uint8Array(await response.arrayBuffer());
        if (data.byteLength === 0) {
          throw new Error("MuJoCo screenshot endpoint returned an empty image.");
        }

        const image = await attachments.saveImage({
          data,
          mediaType: mediaTypeFromHeader(response.headers.get("content-type")),
          name: "mujoco-frame.jpg",
        });

        return {
          image,
          capturedAt: new Date().toISOString(),
          width: image.width,
          height: image.height,
        };
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "get_robot_status",
      description:
        "Read the latest ZRCS robot status, including joint positions, velocities, command positions, " +
        "servo state, scheduler state, and whether the robot is moving. Use it before or after motion when completion matters.",
      parameters: {
        timeout_ms: {
          type: "number",
          description: "HTTP timeout in milliseconds. Defaults to 5000.",
        },
      },
      output: {
        schema: {
          type: "object",
          additionalProperties: false,
          properties: {
            ok: { type: "boolean", required: true },
            available: { type: "boolean", required: true },
            moving: { type: "boolean", required: true },
            motionId: { type: "number", required: true },
            heartbeat: { type: "number", required: true },
            systemState: { type: "string", required: true },
            btTreeState: { type: "string", required: true },
            btCurrentNode: { type: "string", required: true },
            btMessage: { type: "string", required: true },
            positions: { type: "array", items: { type: "number" }, required: true },
            velocities: { type: "array", items: { type: "number" }, required: true },
            commandPositions: { type: "array", items: { type: "number" }, required: true },
            servoEnabled: { type: "array", items: { type: "boolean" }, required: true },
            lastStatusAt: { type: "string", required: true },
            lastMotionAt: { type: "string", required: true },
          },
        },
        render: (_args, value) => [{
          type: "text",
          text: `Robot status: ${value.available ? "available" : "unavailable"}, ` +
            `${value.moving ? "moving" : "stationary"}, scheduler=${value.systemState}, motionId=${value.motionId}.`,
        }],
      },
      async execute(args, exec) {
        return fetchJson("/robot/status", {
          signal: exec?.signal,
          timeoutMs: timeoutValue(args?.timeout_ms),
        });
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "wait_robot_motion",
      description:
        "Wait until the robot has settled after a motion command, using live axis velocities. " +
        "Pass the motionId returned by move_j or move_abs_j. Call this before taking the post-motion screenshot.",
      parameters: {
        motion_id: {
          type: "number",
          description: "The motionId returned by the accepted motion command.",
        },
        timeout_ms: {
          type: "number",
          description: "Maximum wait time in milliseconds. Defaults to 30000.",
        },
        poll_ms: {
          type: "number",
          description: "Status polling interval in milliseconds. Defaults to 100.",
        },
        settle_ms: {
          type: "number",
          description: "How long velocities must remain stopped. Defaults to 300.",
        },
        start_grace_ms: {
          type: "number",
          description: "Grace period for a command to start before accepting a stationary state. Defaults to 500.",
        },
      },
      output: {
        schema: {
          type: "object",
          additionalProperties: false,
          properties: {
            completed: { type: "boolean", required: true },
            motionId: { type: "number", required: true },
            elapsedMs: { type: "number", required: true },
            observedMoving: { type: "boolean", required: true },
            status: { type: "object", additionalProperties: true, required: true },
          },
        },
        render: (_args, value) => [{
          type: "text",
          text: `Robot motion ${value.completed ? "settled" : "not settled"} ` +
            `(motionId=${value.motionId}, ${value.elapsedMs} ms${value.observedMoving ? ", movement observed" : ""}).`,
        }],
      },
      timeoutMs: 120000,
      async execute(args, exec) {
        const requestedMotionId = args?.motion_id === undefined ? undefined : Number(args.motion_id);
        if (requestedMotionId !== undefined &&
            (!Number.isFinite(requestedMotionId) || requestedMotionId < 0)) {
          throw new Error("motion_id must be a non-negative finite number.");
        }

        const timeoutMs = timeoutValue(args?.timeout_ms, 30000, 120000);
        const pollMs = timeoutValue(args?.poll_ms, DEFAULT_POLL_MS, 2000);
        const settleMs = timeoutValue(args?.settle_ms, DEFAULT_SETTLE_MS, 10000);
        const startGraceMs = timeoutValue(args?.start_grace_ms, DEFAULT_START_GRACE_MS, 10000);
        const startedAt = Date.now();
        const deadline = startedAt + timeoutMs;
        let observedMoving = false;
        let stoppedAt;
        let latest;

        while (Date.now() < deadline) {
          latest = await fetchJson("/robot/status", {
            signal: exec?.signal,
            timeoutMs: Math.min(5000, Math.max(1000, pollMs * 2)),
          });
          if (latest.available === false) {
            throw new Error("ZRCS robot status is unavailable; motion completion cannot be verified.");
          }
          if (requestedMotionId !== undefined && Number(latest.motionId) < requestedMotionId) {
            stoppedAt = undefined;
          } else if (latest.moving) {
            observedMoving = true;
            stoppedAt = undefined;
          } else if (Date.now() - startedAt >= startGraceMs) {
            stoppedAt ??= Date.now();
            if (Date.now() - stoppedAt >= settleMs) {
              return {
                completed: true,
                motionId: Number(latest.motionId) || 0,
                elapsedMs: Date.now() - startedAt,
                observedMoving,
                status: latest,
              };
            }
          }

          await sleep(Math.min(pollMs, Math.max(1, deadline - Date.now())), exec?.signal);
        }

        throw new Error(`Robot motion did not settle within ${timeoutMs} ms.`);
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "move_abs_j",
      description:
        "Move the robot to absolute joint positions in radians. " +
        "Use get_mujoco_frame after the motion when visual verification is needed.",
      parameters: {
        positions: {
          type: "array",
          items: { type: "number" },
          required: true,
          description: "Target joint positions [j1,...,jN] in radians.",
        },
      },
      output: motionOutput(),
      timeoutMs: 15000,
      async execute(args, exec) {
        const positions = assertFiniteNumbers(args?.positions, "positions");
        if (positions.length > 15) {
          throw new Error("positions supports at most 15 joints.");
        }
        return postMotion("MoveAbsJ", [positions.length, ...positions], exec?.signal);
      },
    }),
  );

  ctx.tools.register(
    defineTool({
      name: "move_j",
      description:
        "Move the robot in Cartesian space using the ZRCS MoveJ command. " +
        "Pose units are meters and radians; call get_mujoco_frame after the motion to verify it.",
      parameters: {
        x: { type: "number", required: true, description: "Target X in meters." },
        y: { type: "number", required: true, description: "Target Y in meters." },
        z: { type: "number", required: true, description: "Target Z in meters." },
        rx: { type: "number", required: true, description: "Target roll in radians." },
        ry: { type: "number", required: true, description: "Target pitch in radians." },
        rz: { type: "number", required: true, description: "Target yaw in radians." },
        velocity_scale: {
          type: "number",
          description: "Velocity scale from (0, 1]. Defaults to 1.",
        },
      },
      output: motionOutput(),
      timeoutMs: 15000,
      async execute(args, exec) {
        const values = [args?.x, args?.y, args?.z, args?.rx, args?.ry, args?.rz].map(Number);
        if (values.some((value) => !Number.isFinite(value))) {
          throw new Error("MoveJ pose values must be finite numbers.");
        }
        const velocity = args?.velocity_scale === undefined ? 1 : Number(args.velocity_scale);
        if (!Number.isFinite(velocity) || velocity <= 0 || velocity > 1) {
          throw new Error("velocity_scale must be greater than 0 and no greater than 1.");
        }
        return postMotion("MoveJ", [...values, velocity], exec?.signal);
      },
    }),
  );

  ctx.logger?.info?.("dsh-plugin-zrcs-mujoco loaded (on-demand screenshots)");
}
