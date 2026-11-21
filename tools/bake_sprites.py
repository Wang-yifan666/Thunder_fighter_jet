"""把 PNG 素材烘焙成可内嵌的原始 RGBA 数据。

项目约定
--------
* `assets/sprites/<name>.png` 是**美术母版**（透明底，内容居中）。
* 本脚本按目标尺寸等比缩放内容（不拉伸），写出 `<name>_<w>x<h>.rgba`
  （逐像素 R,G,B,A 直通 alpha，行优先，无文件头）。
* CMake 用 `file(READ ... HEX)` 把 `.rgba` 变成 C++ 字节数组编进可执行文件，
  因此发布仍然只需要一个 exe（与内嵌字体同一套机制）。

关于"边缘发黑"
--------------
部分素材是从纯黑背景抠出 alpha 的，边上的像素颜色被 alpha 乘过一次
（预乘 over black），直接当直通 alpha 用会出现一圈黑边。
`edge_fringe()` 会测量这一点，必要时做反预乘还原。
测量结果是"边缘 / 实心"的平均亮度比：干净素材接近 1，发黑的素材远小于 1。

为什么不用 PIL
--------------
开发机不要求任何第三方 Python 包；这里只用标准库 zlib 手写 PNG 解码。

用法
----
    python tools/bake_sprites.py            # 按脚本内的默认尺寸烘焙全部
    python tools/bake_sprites.py --size 48  # 覆盖正方形素材的尺寸
"""

import argparse
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SPRITE_DIR = os.path.join(ROOT, "assets", "sprites")

# 母版文件名 -> [(输出基础名, 输出宽, 输出高), ...]
# 尺寸 = 游戏里实际绘制的像素尺寸，按 GameConfig::kCellSize 与格子数换算。
# 一个母版可以出多份产物（例如护盾泡：小怪用 32，精英和玩家用 48）。
SPRITES = {
    "player.png": [("player", 32, 32)],  # 3x1 格命中框，精灵 4 格宽、底边对齐
    "enemy.png": [("enemy", 24, 16)],    # 3x2 格命中框，精灵与判定等大
    "elite.png": [("elite", 32, 24)],    # 精英：4x3 格
    "health_pack.png": [("health", 24, 24)],  # 3x3 格，精灵与判定等大
    "ammo_pack.png": [("ammo", 24, 24)],      # 3x3 格，精灵与判定等大
    "shield_pack.png": [("shield_pack", 24, 24)],
    "firepower_pack.png": [("firepower", 24, 24)],
    # 护盾泡：小怪 40x40 罩住 3x2（留一圈余量）；精英 4x3 和玩家 32x32 精灵都用 56x56
    "shield_bubble.png": [("shield_bubble", 40, 40), ("shield_bubble_big", 56, 56)],
}

# 已经在全分辨率做过反预乘的母版：烘焙时不再重复检测，
# 否则会把已经还原过的边缘再提亮一次。
PREFIXED = {"health_pack.png"}

DEFAULT_SIZE = 32

# 边缘亮度比低于这个值就认为素材带黑边，需要反预乘
FRINGE_THRESHOLD = 0.60


def read_png(path):
    """解码 8 位 RGB/RGBA 非交错 PNG，返回 (宽, 高, 通道数, 像素字节)。"""
    with open(path, "rb") as handle:
        data = handle.read()

    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s 不是 PNG 文件" % path)

    pos = 8
    idat = bytearray()
    width = height = depth = color = interlace = None

    while pos < len(data):
        length = struct.unpack(">I", data[pos:pos + 4])[0]
        kind = data[pos + 4:pos + 8]
        payload = data[pos + 8:pos + 8 + length]
        pos += 12 + length

        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(
                ">IIBBBBB", payload)
        elif kind == b"IDAT":
            idat += payload
        elif kind == b"IEND":
            break

    if depth != 8:
        raise ValueError("%s：只支持 8 位深度（当前 %d）" % (path, depth))
    if color not in (2, 6):
        raise ValueError("%s：只支持 RGB/RGBA（当前颜色类型 %d）" % (path, color))
    if interlace != 0:
        raise ValueError("%s：不支持交错 PNG" % path)

    channels = 4 if color == 6 else 3
    stride = width * channels
    raw = zlib.decompress(bytes(idat))

    out = bytearray(stride * height)
    prev = bytearray(stride)
    src = 0
    for y in range(height):
        filter_type = raw[src]
        src += 1
        line = bytearray(raw[src:src + stride])
        src += stride

        if filter_type == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif filter_type == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif filter_type == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif filter_type == 4:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                if pa <= pb and pa <= pc:
                    pred = a
                elif pb <= pc:
                    pred = b
                else:
                    pred = c
                line[i] = (line[i] + pred) & 0xFF
        elif filter_type != 0:
            raise ValueError("%s：未知行过滤类型 %d" % (path, filter_type))

        out[y * stride:(y + 1) * stride] = line
        prev = line

    if channels == 4:
        return width, height, channels, out

    rgba = bytearray(width * height * 4)
    for i in range(width * height):
        rgba[i * 4] = out[i * 3]
        rgba[i * 4 + 1] = out[i * 3 + 1]
        rgba[i * 4 + 2] = out[i * 3 + 2]
        rgba[i * 4 + 3] = 255
    return width, height, 4, rgba


def edge_fringe(width, height, pixels):
    """返回 (边缘/实心平均亮度比, 边缘像素数)。越接近 1 越干净。"""
    edge_sum = edge_count = 0
    solid_sum = solid_count = 0
    for i in range(0, width * height * 4, 4):
        alpha = pixels[i + 3]
        lum = (pixels[i] + pixels[i + 1] + pixels[i + 2]) / 3.0
        if 16 < alpha < 128:
            edge_sum += lum
            edge_count += 1
        elif alpha >= 250:
            solid_sum += lum
            solid_count += 1

    if not edge_count or not solid_count:
        return None, 0
    return (edge_sum / edge_count) / (solid_sum / solid_count), edge_count


def unpremultiply(width, height, pixels):
    """反预乘：RGB / (a/255)，把"被压黑"的边缘颜色还原。"""
    out = bytearray(pixels)
    for i in range(0, width * height * 4, 4):
        alpha = out[i + 3]
        if 0 < alpha < 250:
            factor = 255.0 / alpha
            for k in range(3):
                value = int(out[i + k] * factor)
                out[i + k] = 255 if value > 255 else value
    return out


def content_bbox(width, height, pixels):
    min_x, min_y, max_x, max_y = width, height, -1, -1
    for y in range(height):
        base = y * width * 4
        for x in range(width):
            if pixels[base + x * 4 + 3] > 8:
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y
    if max_x < 0:
        raise ValueError("图片完全透明，没有可烘焙的内容")
    return min_x, min_y, max_x - min_x + 1, max_y - min_y + 1


def fit_into(width, height, pixels, box_w, box_h):
    """把内容按比例缩放到 box_w x box_h 内并居中（多余部分透明）。"""
    bx, by, bw, bh = content_bbox(width, height, pixels)

    scale = min(box_w / float(bw), box_h / float(bh))
    tw = max(1, int(round(bw * scale)))
    th = max(1, int(round(bh * scale)))
    off_x = (box_w - tw) // 2
    off_y = (box_h - th) // 2

    out = [[(0, 0, 0, 0)] * box_w for _ in range(box_h)]
    for dy in range(th):
        sy0 = by + dy * bh // th
        sy1 = max(sy0 + 1, by + (dy + 1) * bh // th)
        for dx in range(tw):
            sx0 = bx + dx * bw // tw
            sx1 = max(sx0 + 1, bx + (dx + 1) * bw // tw)
            r = g = b = a = count = 0
            for sy in range(sy0, min(sy1, by + bh)):
                row = sy * width * 4
                for sx in range(sx0, min(sx1, bx + bw)):
                    i = row + sx * 4
                    pa = pixels[i + 3]
                    r += pixels[i] * pa
                    g += pixels[i + 1] * pa
                    b += pixels[i + 2] * pa
                    a += pa
                    count += 1
            if count and a:
                out[off_y + dy][off_x + dx] = (r // a, g // a, b // a,
                                               a // count)
    return out


def bake(name, size_override, fringe_report):
    if name not in SPRITES:
        print("未登记素材：%s" % name, file=sys.stderr)
        return []

    src_path = os.path.join(SPRITE_DIR, name)
    if not os.path.exists(src_path):
        print("跳过（母版不存在）：%s" % src_path)
        return []

    width, height, _, pixels = read_png(src_path)

    if name not in PREFIXED:
        ratio, edge_count = edge_fringe(width, height, pixels)
        if ratio is not None and ratio < FRINGE_THRESHOLD:
            pixels = unpremultiply(width, height, pixels)
            after, _ = edge_fringe(width, height, pixels)
            fringe_report.append((name, ratio, after, edge_count))

    built = []
    for base, out_w, out_h in SPRITES[name]:
        if size_override and out_w == out_h:
            out_w = out_h = size_override

        small = fit_into(width, height, pixels, out_w, out_h)

        flat = bytearray()
        for row in small:
            for r, g, b, a in row:
                flat.extend((r, g, b, a))

        out_path = os.path.join(SPRITE_DIR,
                                "%s_%dx%d.rgba" % (base, out_w, out_h))
        with open(out_path, "wb") as handle:
            handle.write(bytes(flat))

        opaque = sum(1 for i in range(3, len(flat), 4) if flat[i] > 8)
        print("%-18s %4dx%-4d -> %-26s %5d B  覆盖 %5.1f%%"
              % (name, width, height, os.path.basename(out_path),
                 len(flat), 100.0 * opaque / (out_w * out_h)))
        built.append(out_path)
    return built


def main():
    parser = argparse.ArgumentParser(description="烘焙精灵素材")
    parser.add_argument("--size", type=int, default=0,
                        help="覆盖正方形素材的输出边长（默认按 SPRITES 表）")
    args = parser.parse_args()

    if not os.path.isdir(SPRITE_DIR):
        print("找不到素材目录：%s" % SPRITE_DIR, file=sys.stderr)
        return 1

    fringe_report = []
    built = 0
    for name in sorted(SPRITES):
        if bake(name, args.size, fringe_report):
            built += 1

    if fringe_report:
        print("\n边缘发黑已修正（边缘/实心亮度比）：")
        for name, before, after, count in fringe_report:
            tail = "已修复" if after and after > FRINGE_THRESHOLD else "仅部分修复"
            print("  %-14s %.2f -> %.2f  (%d 个边缘像素)  %s"
                  % (name, before, after or 0.0, count, tail))

    if built == 0:
        print("没有任何素材被烘焙。", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
