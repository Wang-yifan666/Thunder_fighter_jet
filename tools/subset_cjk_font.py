"""把中文字体子集化，只保留游戏里真正会用到的汉字。

为什么不用系统里的微软雅黑之类的字体？
--------------------------------------
那些字体不允许随程序再分发（连"打散的位图字形"也不行）。这里必须用
**可以再分发的开源字体**，默认取 Noto Sans SC（SIL Open Font License 1.1）。

语料从哪来？
------------
直接从 inc/game/game_text.h 里抽取所有非 ASCII 字符——文案是唯一来源，
加了新文案就重跑一次，不会漏字。

用法：
    python tools/subset_cjk_font.py
    python tools/subset_cjk_font.py --source "C:/Windows/Fonts/NotoSansSC-VF.ttf"

前置：pip install fonttools brotli
产物：assets/fonts/NotoSansSC-Subset.ttf（构建时嵌入 exe）
"""

import argparse
import os
import re
import sys

from fontTools import subset
from fontTools.ttLib import TTFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEXT_HEADER = os.path.join(ROOT, "inc", "game", "game_text.h")
OUT_PATH = os.path.join(ROOT, "assets", "fonts", "NotoSansSC-Subset.ttf")

DEFAULT_SOURCES = [
    r"C:\Windows\Fonts\NotoSansSC-VF.ttf",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/System/Library/Fonts/NotoSansSC.ttf",
]

# 这几个字符不出现中文，但界面上要用（全角冒号、空格、破折号等），显式补上
EXTRA_CHARS = "　：，。！？（）、"


def collect_chars(header_path):
    """只取代码里的字面量：注释里写的中文不进子集，否则会白白带进一堆字形。"""
    with open(header_path, encoding="utf-8") as handle:
        source = handle.read()

    # 去掉 // 行注释和 /* */ 块注释（够用的简易版，够处理我们自己的头文件）
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
    source = re.sub(r"//[^\n]*", "", source)

    chars = {ch for ch in source if ord(ch) > 0x7F}
    chars.update(EXTRA_CHARS)
    return chars


def pick_source(explicit):
    if explicit:
        if not os.path.exists(explicit):
            raise SystemExit("找不到字体文件：%s" % explicit)
        return explicit
    for path in DEFAULT_SOURCES:
        if os.path.exists(path):
            return path
    raise SystemExit("找不到可用的中文字体，请用 --source 指定 Noto Sans SC 的路径")


def main():
    parser = argparse.ArgumentParser(description="按文案子集化中文字体")
    parser.add_argument("--source", default="", help="源字体路径（默认自动找 Noto Sans SC）")
    parser.add_argument("--out", default=OUT_PATH)
    args = parser.parse_args()

    source = pick_source(args.source)
    chars = collect_chars(TEXT_HEADER)
    text = "".join(sorted(chars))

    print("源字体：%s" % source)
    print("需要 %d 个字形：%s" % (len(chars), text))

    font = TTFont(source)
    subsetter = subset.Subsetter()
    subsetter.populate(text=text)
    subsetter.subset(font)

    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    font.save(args.out)
    size = os.path.getsize(args.out)
    print("写出 %s  %.1f KB" % (args.out, size / 1024.0))

    # 体积上限做个提醒：超过 300 KB 说明语料里混进了不该有的东西
    if size > 300 * 1024:
        print("警告：子集超过 300 KB，检查一下 game_text.h 是不是混入了大段文本", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
