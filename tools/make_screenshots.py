"""生成文档截图。

流程：用无头模式跑 Naidan.exe（`--screenshot` 把最后一帧写成原始 RGBA），
再把 RGBA 转成 PNG 放到 assets/screenshots/。

这样截图永远和当前代码一致——改完界面重跑一条命令即可，不会像以前那样
留着一张几个月前的旧图。

用法：
    python tools/make_screenshots.py
    python tools/make_screenshots.py --exe build/Naidan.exe --out assets/screenshots
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# (文件名, 帧数, 是否用脚本化输入)
SHOTS = [
    ("menu", 1, False),
    ("gameplay", 200, True),
    ("gameover", 340, True),
]


def read_raw_rgba(path):
    """读取 --screenshot 产出的格式："宽 高\\n" + RGBA 像素。"""
    with open(path, "rb") as handle:
        header = handle.readline().split()
        width, height = int(header[0]), int(header[1])
        pixels = handle.read()
    expected = width * height * 4
    if len(pixels) != expected:
        raise ValueError(
            "%s 像素数不符：期望 %d，实际 %d" % (path, expected, len(pixels)))
    return width, height, pixels


def write_png(path, width, height, pixels):
    """把 RGBA 像素写成 PNG（不透明度 255 时退化为 RGB 以减小体积）。"""
    # 判断是否所有像素都不透明
    opaque = all(pixels[i] == 255 for i in range(3, len(pixels), 4))
    channels = 3 if opaque else 4

    raw = bytearray()
    for y in range(height):
        raw.append(0)  # 行过滤：None
        base = y * width * 4
        if channels == 3:
            for x in range(width):
                i = base + x * 4
                raw.extend(pixels[i:i + 3])
        else:
            raw.extend(pixels[base:base + width * 4])

    def chunk(tag, payload):
        body = tag + payload
        return (struct.pack(">I", len(payload)) + body
                + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF))

    color_type = 2 if channels == 3 else 6
    ihdr = struct.pack(">IIBBBBB", width, height, 8, color_type, 0, 0, 0)
    data = (b"\x89PNG\r\n\x1a\n"
            + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
            + chunk(b"IEND", b""))
    with open(path, "wb") as handle:
        handle.write(data)
    return len(data)


def main():
    parser = argparse.ArgumentParser(description="生成文档截图")
    parser.add_argument("--exe", default=os.path.join(ROOT, "build", "Naidan.exe"))
    parser.add_argument("--out", default=os.path.join(ROOT, "assets", "screenshots"))
    parser.add_argument("--seed", type=int, default=20261107,
                        help="固定随机种子，保证截图可复现")
    args = parser.parse_args()

    if not os.path.exists(args.exe):
        print("找不到可执行文件：%s（先构建）" % args.exe, file=sys.stderr)
        return 1

    out_dir = os.path.abspath(args.out)
    os.makedirs(out_dir, exist_ok=True)

    env = dict(os.environ)
    env["SDL_VIDEODRIVER"] = "dummy"

    # 排行榜存档在 exe 同目录，会被上一轮演示改写；
    # HUD 里的 BEST 依赖它，所以每次生成前先清掉，保证截图可复现。
    score_file = os.path.join(os.path.dirname(os.path.abspath(args.exe)), "highscore.txt")
    if os.path.exists(score_file):
        os.remove(score_file)

    # 设置文件（教程进度 / 装扮）同样会影响界面（比如装扮是否解锁），一并清掉
    settings_file = os.path.join(os.path.dirname(os.path.abspath(args.exe)), "settings.txt")
    if os.path.exists(settings_file):
        os.remove(settings_file)

    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, frames, demo in SHOTS:
            raw_path = os.path.join(tmp, name + ".rgba")
            cmd = [args.exe, "--frames", str(frames), "--screenshot", raw_path,
                   "--seed", str(args.seed)]
            if demo:
                cmd.append("--demo")

            result = subprocess.run(cmd, env=env, capture_output=True, text=True)
            if not os.path.exists(raw_path):
                print("失败：%s（exit=%d）%s"
                      % (name, result.returncode, result.stderr.strip()),
                      file=sys.stderr)
                failed += 1
                continue

            width, height, pixels = read_raw_rgba(raw_path)
            png_path = os.path.join(out_dir, name + ".png")
            size = write_png(png_path, width, height, pixels)
            print("%-10s %dx%d  %6d B  -> %s"
                  % (name, width, height, size, os.path.relpath(png_path, ROOT)))

    if failed:
        print("%d 张截图失败" % failed, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
