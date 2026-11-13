"""Renders host-harness recordings (*.txt) to PNG: the Tracker's 800x480
screen plus a strip showing which of the 48 pads are lit."""
import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from fonts import FONTS

# i2's 5x7 dot face: the same table the firmware uses (i2DotFont.cpp)
DOT_PITCH = [2, 3, 6]
DOT_LIT = [2, 2, 5]


def load_dot_table():
    src = (Path(__file__).resolve().parents[3] / "core/interface/i2Deck/i2DotFont.cpp").read_text()
    rows = re.findall(r"\{\s*(0x[0-9A-Fa-f]{2}),\s*(0x[0-9A-Fa-f]{2}),\s*(0x[0-9A-Fa-f]{2}),\s*(0x[0-9A-Fa-f]{2}),\s*(0x[0-9A-Fa-f]{2})\s*\}", src)
    assert len(rows) == 64, len(rows)
    return [[int(v, 16) for v in r] for r in rows]


DOT = load_dot_table()

FONT_CACHE = {}


def font(fid):
    if fid not in FONT_CACHE:
        path, size, _ = FONTS.get(fid, FONTS[0])
        FONT_CACHE[fid] = ImageFont.truetype(path, size)
    return FONT_CACHE[fid]


def rgb(h, a=255):
    v = int(h, 16)
    return ((v >> 16) & 255, (v >> 8) & 255, v & 255, a)


def render(txt_path, png_path):
    W, H = 800, 480
    img = Image.new("RGBA", (W, H + 90), (0, 0, 0, 255))
    col = "ffffff"
    alpha = 255
    clip = None
    labels = {}
    pads = ""
    name = ""

    def draw_prim(fn):
        layer = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        fn(ImageDraw.Draw(layer))
        if clip:
            x, y, w, h = clip
            mask = Image.new("L", (W, H), 0)
            ImageDraw.Draw(mask).rectangle([x, y, x + w - 1, y + h - 1], fill=255)
            layer.putalpha(Image.composite(layer.getchannel("A"), mask, mask))
        img.alpha_composite(layer, (0, 0))

    for raw in Path(txt_path).read_text().splitlines():
        if not raw or raw.startswith("#"):
            continue
        k = raw[0]
        if k == "C":
            col = raw[2:].strip()
        elif k == "A":
            alpha = int(raw[2:])
        elif k == "R":
            x0, y0, x1, y1, r = map(int, raw[2:].split())
            c = rgb(col, alpha)
            draw_prim(lambda d: d.rounded_rectangle([x0, y0, x1 - 1, y1 - 1], radius=r, fill=c) if r else d.rectangle([x0, y0, x1 - 1, y1 - 1], fill=c))
        elif k == "O":
            cx, cy, r = map(int, raw[2:].split())
            c = rgb(col, alpha)
            draw_prim(lambda d: d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=c))
        elif k == "Q":
            v = list(map(int, raw[2:].split()))
            r, n, xy = v[0], v[1], v[2:]
            c = rgb(col, alpha)
            def pts(d, xy=xy, r=r, c=c):
                for i in range(0, len(xy), 2):
                    d.ellipse([xy[i] - r, xy[i + 1] - r, xy[i] + r, xy[i + 1] + r], fill=c)
            draw_prim(pts)
        elif k == "F":
            v = list(map(int, raw[2:].split()))
            xy = v[1:]
            bottom = (clip[1] + clip[3]) if clip else H
            poly = [(xy[i], xy[i + 1]) for i in range(0, len(xy), 2)]
            poly = poly + [(poly[-1][0], bottom), (poly[0][0], bottom)]
            c = rgb(col, alpha)
            draw_prim(lambda d: d.polygon(poly, fill=c))
        elif k == "L":
            x0, y0, x1, y1, w = map(int, raw[2:].split())
            c = rgb(col, alpha)
            draw_prim(lambda d: d.line([x0, y0, x1, y1], fill=c, width=max(1, w)))
        elif k == "T":
            head, text = raw[2:].split("|", 1)
            x, y, fid, align, n = map(int, head.split())
            f = font(fid)
            w = f.getlength(text)
            if align & 1:
                x -= w / 2
            elif align & 2:
                x -= w
            if align & 4:
                y -= FONTS.get(fid, FONTS[0])[2] / 2
            c = rgb(col, alpha)
            draw_prim(lambda d: d.text((x, y), text, font=f, fill=c))
        elif k == "D":
            head, text = raw[2:].split("|", 1)
            x, y, size, align, n = map(int, head.split())
            pitch, lit = DOT_PITCH[size], DOT_LIT[size]
            w = len(text) * 6 * pitch
            if align & 1:
                x -= w // 2
            elif align & 2:
                x -= w
            c = rgb(col, alpha)
            def dots(d, x=x, y=y, text=text, pitch=pitch, lit=lit, c=c):
                for i, ch in enumerate(text.upper()):
                    o = ord(ch)
                    cols = DOT[o - 32] if 32 <= o < 96 else DOT[0]
                    for cx in range(5):
                        for cy in range(7):
                            if cols[cx] >> cy & 1:
                                px, py = x + i * 6 * pitch + cx * pitch, y + cy * pitch
                                d.rectangle([px, py, px + lit - 1, py + lit - 1], fill=c)
            draw_prim(dots)
        elif k == "S":
            clip = tuple(map(int, raw[2:].split()))
        elif k == "U":
            clip = None
        elif k == "B":
            parts = raw[2:].split(" ", 1)
            labels[int(parts[0])] = parts[1] if len(parts) > 1 else ""
        elif k == "P":
            pads = raw[2:].strip()
        elif k == "N":
            name = raw[2:].strip()

    d = ImageDraw.Draw(img)
    # pad strip
    for p, ch in enumerate(pads[:48]):
        r, c = divmod(p, 12)
        lvl = int(ch)
        fill = [(30, 30, 30, 255), (110, 110, 90, 255), (255, 255, 210, 255)][lvl]
        x0, y0 = 10 + c * 65, H + 6 + r * 20
        d.rectangle([x0, y0, x0 + 58, y0 + 15], fill=fill)
    img.convert("RGB").save(png_path)


if __name__ == "__main__":
    for p in sys.argv[1:]:
        src = Path(p)
        if not src.read_text().startswith("# layer"):
            continue
        render(src, src.with_suffix(".png"))
