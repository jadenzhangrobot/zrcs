r"""
Smoke test for a local OpenVINO Qwen2.5-VL model.

Default run:
    E:\vla\Scripts\python.exe test_qwen25_vl_ov.py

With your own image:
    E:\vla\Scripts\python.exe test_qwen25_vl_ov.py --image E:\path\to\image.jpg

With laptop camera:
    E:\vla\Scripts\python.exe test_qwen25_vl_ov.py --camera

Try another OpenVINO device:
    E:\vla\Scripts\python.exe test_qwen25_vl_ov.py --device AUTO
"""

from __future__ import annotations

import argparse
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont
from qwen_vl_utils import process_vision_info
from transformers import AutoProcessor

from optimum.intel.openvino import OVModelForVisualCausalLM


DEFAULT_MODEL_DIR = Path(r"E:\Qwen2.5_VL_3B_OV")
DEFAULT_INFERENCE_DEVICE = "GPU"
DEFAULT_IMAGE_PROMPT = "请用中文描述图片里的主要内容，包括人物、物体、文字、颜色和大致位置。"
DEFAULT_COORDINATE_PROMPT = (
    "Find visible objects or shapes in the image. "
    "Output only a JSON array, without Markdown code fences. "
    "Each item must have keys label, bbox, and center. "
    "bbox must be [x1, y1, x2, y2], center must be [cx, cy], "
    "using pixel coordinates relative to the image. If unsure, estimate."
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run a simple generation test for Qwen2.5-VL OpenVINO model."
    )
    parser.add_argument(
        "--model-dir",
        type=Path,
        default=DEFAULT_MODEL_DIR,
        help=f"Path to OpenVINO model directory. Default: {DEFAULT_MODEL_DIR}",
    )
    parser.add_argument(
        "--image",
        type=Path,
        default=None,
        help="Optional image path. If omitted, a temporary test image is generated.",
    )
    parser.add_argument(
        "--camera",
        action="store_true",
        help="Capture one frame from the laptop camera and use it as the image input.",
    )
    parser.add_argument(
        "--camera-loop",
        action="store_true",
        help="Continuously capture camera frames and run inference for each frame.",
    )
    parser.add_argument(
        "--coordinate-output",
        action="store_true",
        help="Output estimated object coordinates instead of a Chinese image description.",
    )
    parser.add_argument(
        "--camera-index",
        type=int,
        default=0,
        help="Camera index for OpenCV VideoCapture. Default: 0.",
    )
    parser.add_argument(
        "--camera-output",
        type=Path,
        default=None,
        help="Optional path to save the captured camera frame.",
    )
    parser.add_argument(
        "--camera-warmup-frames",
        type=int,
        default=10,
        help="Frames to discard before saving the camera image. Default: 10.",
    )
    parser.add_argument(
        "--camera-save-dir",
        type=Path,
        default=None,
        help="Optional directory to save frames captured in --camera-loop mode.",
    )
    parser.add_argument(
        "--frames",
        type=int,
        default=0,
        help="Number of frames to process in --camera-loop mode. 0 means run until Ctrl+C.",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.0,
        help="Seconds to wait between camera-loop inferences. Default: 0.",
    )
    parser.add_argument(
        "--prompt",
        default=None,
        help=(
            "User prompt sent to the model. Defaults to a Chinese image description "
            "prompt, or to a coordinate JSON prompt when --coordinate-output is set."
        ),
    )
    parser.add_argument(
        "--device",
        default=DEFAULT_INFERENCE_DEVICE,
        help=(
            "OpenVINO device, for example CPU, GPU, NPU, or AUTO. "
            f"Default: {DEFAULT_INFERENCE_DEVICE}."
        ),
    )
    parser.add_argument(
        "--max-new-tokens",
        type=int,
        default=96,
        help="Maximum number of tokens to generate. Default: 96.",
    )
    parser.add_argument(
        "--text-only",
        action="store_true",
        help="Skip image input and test only text generation.",
    )
    return parser


def import_cv2() -> Any:
    try:
        import cv2  # type: ignore[import-not-found]
    except ImportError as exc:
        raise RuntimeError(
            "Camera mode needs OpenCV. Install it with: "
            r"E:\vla\Scripts\python.exe -m pip install opencv-python"
        ) from exc
    return cv2


def make_test_image() -> Path:
    temp_dir = Path(tempfile.mkdtemp(prefix="qwen25_vl_test_"))
    image_path = temp_dir / "sample.png"

    image = Image.new("RGB", (640, 420), "white")
    draw = ImageDraw.Draw(image)

    draw.rectangle((40, 40, 250, 190), fill=(237, 74, 64), outline="black", width=4)
    draw.ellipse((340, 50, 570, 210), fill=(70, 145, 255), outline="black", width=4)
    draw.polygon([(120, 330), (250, 230), (380, 330)], fill=(60, 180, 100), outline="black")

    try:
        font = ImageFont.truetype("arial.ttf", 42)
    except OSError:
        font = ImageFont.load_default()

    draw.text((58, 92), "OPENVINO", fill="white", font=font)
    draw.text((375, 105), "Qwen", fill="white", font=font)
    draw.text((160, 355), "VL test", fill="black", font=font)

    image.save(image_path)
    return image_path


def capture_camera_image(camera_index: int, output_path: Path | None, warmup_frames: int) -> Path:
    cv2 = import_cv2()

    if output_path is None:
        temp_dir = Path(tempfile.mkdtemp(prefix="qwen25_vl_camera_"))
        output_path = temp_dir / "camera.png"
    else:
        output_path = output_path.expanduser().resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)

    cap = cv2.VideoCapture(camera_index, cv2.CAP_DSHOW)
    if not cap.isOpened():
        cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open camera index {camera_index}.")

    frame = None
    try:
        frames_to_read = max(1, warmup_frames + 1)
        for _ in range(frames_to_read):
            ok, current_frame = cap.read()
            if ok:
                frame = current_frame
        if frame is None:
            raise RuntimeError(f"Could not read a frame from camera index {camera_index}.")
    finally:
        cap.release()

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    Image.fromarray(rgb_frame).save(output_path)
    return output_path


def open_camera(cv2: Any, camera_index: int) -> Any:
    cap = cv2.VideoCapture(camera_index, cv2.CAP_DSHOW)
    if not cap.isOpened():
        cap = cv2.VideoCapture(camera_index)
    if not cap.isOpened():
        raise RuntimeError(f"Could not open camera index {camera_index}.")
    return cap


def save_camera_frame(cv2: Any, cap: Any, output_path: Path, warmup_frames: int) -> Path:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    frame = None

    frames_to_read = max(1, warmup_frames + 1)
    for _ in range(frames_to_read):
        ok, current_frame = cap.read()
        if ok:
            frame = current_frame

    if frame is None:
        raise RuntimeError("Could not read a frame from the camera.")

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    Image.fromarray(rgb_frame).save(output_path)
    return output_path


def as_qwen_image_value(path: Path) -> str:
    return str(path.expanduser().resolve())


def build_messages(prompt: str, image_path: Path | None) -> list[dict[str, Any]]:
    content: list[dict[str, Any]] = []
    if image_path is not None:
        content.append({"type": "image", "image": as_qwen_image_value(image_path)})
    content.append({"type": "text", "text": prompt})
    return [{"role": "user", "content": content}]


def prepare_inputs(processor: Any, messages: list[dict[str, Any]], text_only: bool) -> Any:
    text = processor.apply_chat_template(
        messages,
        tokenize=False,
        add_generation_prompt=True,
    )

    processor_kwargs: dict[str, Any] = {
        "text": [text],
        "padding": True,
        "return_tensors": "pt",
    }

    if not text_only:
        image_inputs, video_inputs = process_vision_info(messages)
        if image_inputs is not None:
            processor_kwargs["images"] = image_inputs
        if video_inputs is not None:
            processor_kwargs["videos"] = video_inputs

    return processor(**processor_kwargs)


def decode_new_tokens(processor: Any, inputs: Any, generated_ids: Any) -> str:
    trimmed_ids = [
        output_ids[len(input_ids) :]
        for input_ids, output_ids in zip(inputs.input_ids, generated_ids)
    ]
    decoded = processor.batch_decode(
        trimmed_ids,
        skip_special_tokens=True,
        clean_up_tokenization_spaces=False,
    )
    return decoded[0].strip()


def generate_answer(
    model: Any,
    processor: Any,
    prompt: str,
    image_path: Path | None,
    max_new_tokens: int,
    text_only: bool = False,
) -> tuple[str, float]:
    messages = build_messages(prompt, image_path)
    inputs = prepare_inputs(processor, messages, text_only=text_only)

    generation_started = time.perf_counter()
    generated_ids = model.generate(
        **inputs,
        max_new_tokens=max_new_tokens,
    )
    elapsed = time.perf_counter() - generation_started
    return decode_new_tokens(processor, inputs, generated_ids), elapsed


def strip_code_fence(text: str) -> str:
    stripped = text.strip()
    if not stripped.startswith("```"):
        return stripped

    lines = stripped.splitlines()
    if lines and lines[0].lstrip().startswith("```"):
        lines = lines[1:]
    if lines and lines[-1].strip() == "```":
        lines = lines[:-1]
    return "\n".join(lines).strip()


def run_camera_loop(model: Any, processor: Any, args: argparse.Namespace, prompt: str) -> None:
    cv2 = import_cv2()
    save_dir = (
        args.camera_save_dir.expanduser().resolve()
        if args.camera_save_dir
        else Path(tempfile.mkdtemp(prefix="qwen25_vl_camera_loop_"))
    )

    cap = open_camera(cv2, args.camera_index)
    print(f"Camera loop save dir: {save_dir}")
    print("Press Ctrl+C to stop.")

    frame_no = 0
    try:
        while args.frames <= 0 or frame_no < args.frames:
            frame_no += 1
            image_path = save_dir / f"frame_{frame_no:06d}.png"
            save_camera_frame(
                cv2=cv2,
                cap=cap,
                output_path=image_path,
                warmup_frames=args.camera_warmup_frames if frame_no == 1 else 0,
            )

            answer, elapsed = generate_answer(
                model=model,
                processor=processor,
                prompt=prompt,
                image_path=image_path,
                max_new_tokens=args.max_new_tokens,
            )
            answer = strip_code_fence(answer)
            timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
            print(f"\n[{timestamp}] frame={frame_no} image={image_path}")
            print(answer)
            print(f"Generated in {elapsed:.2f}s")

            if args.interval > 0:
                time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nStopped camera loop.")
    finally:
        cap.release()


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    model_dir = args.model_dir.expanduser().resolve()
    prompt = args.prompt or (DEFAULT_COORDINATE_PROMPT if args.coordinate_output else DEFAULT_IMAGE_PROMPT)

    if args.camera_loop:
        args.camera = True

    if args.image is not None and args.camera:
        parser.error("--image and --camera cannot be used together.")
    if args.text_only and (args.image is not None or args.camera):
        parser.error("--text-only cannot be used with --image or --camera.")

    if not model_dir.exists():
        raise FileNotFoundError(f"Model directory does not exist: {model_dir}")

    if args.camera_loop:
        import_cv2()

    image_path = None
    if not args.text_only and not args.camera_loop:
        if args.camera:
            image_path = capture_camera_image(
                camera_index=args.camera_index,
                output_path=args.camera_output,
                warmup_frames=args.camera_warmup_frames,
            )
        elif args.image:
            image_path = args.image.expanduser().resolve()
        else:
            image_path = make_test_image()
        if not image_path.exists():
            raise FileNotFoundError(f"Image does not exist: {image_path}")

    print(f"Model: {model_dir}")
    print(f"Device: {args.device}")
    if image_path is not None:
        print(f"Image:  {image_path}")
    print(f"Prompt: {prompt}")

    load_started = time.perf_counter()
    model_path = str(model_dir)
    processor = AutoProcessor.from_pretrained(
        model_path,
        trust_remote_code=True,
        local_files_only=True,
        fix_mistral_regex=True,
    )
    model = OVModelForVisualCausalLM.from_pretrained(
        model_path,
        device=args.device,
        trust_remote_code=True,
        compile=True,
    )
    print(f"Loaded in {time.perf_counter() - load_started:.2f}s")

    if args.camera_loop:
        run_camera_loop(model, processor, args, prompt)
        return 0

    answer, elapsed = generate_answer(
        model=model,
        processor=processor,
        prompt=prompt,
        image_path=image_path,
        max_new_tokens=args.max_new_tokens,
        text_only=args.text_only,
    )
    print("\nAnswer:")
    print(answer)
    print(f"\nGenerated in {elapsed:.2f}s")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        raise SystemExit(1)
