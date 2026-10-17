#!/usr/bin/env python3
"""Render UTF-8 experiment logs as terminal-style PNG evidence."""

from pathlib import Path
import sys

from PIL import Image, ImageDraw, ImageFont


def load_font(size: int):
    for path in (
        Path("C:/Windows/Fonts/consola.ttf"),
        Path("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"),
    ):
        if path.exists():
            return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


def main():
    if len(sys.argv) not in (4, 5):
        raise SystemExit("usage: render_test_evidence.py LOG PNG TITLE [START]")

    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    title = sys.argv[3]
    content = source.read_text(encoding="utf-8", errors="replace")
    if len(sys.argv) == 5 and sys.argv[4] in content:
        content = content[content.index(sys.argv[4]) :]
    lines = [line.expandtabs(4)[:118] for line in content.rstrip().splitlines()]
    lines = lines or ["(empty output)"]

    body_font = load_font(18)
    image = Image.new("RGB", (1500, 92 + 25 * len(lines)), "#0c0f14")
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, image.width, 48), fill="#222831")
    for x, color in ((18, "#ff5f57"), (40, "#febc2e"), (62, "#28c840")):
        draw.ellipse((x, 17, x + 14, 31), fill=color)
    draw.text((96, 13), title, font=body_font, fill="#e6edf3")

    y = 62
    for line in lines:
        color = "#7ee787" if "PASS" in line or "20/20" in line else "#d6deeb"
        draw.text((20, y), line, font=body_font, fill=color)
        y += 25

    output.parent.mkdir(parents=True, exist_ok=True)
    image.save(output)


if __name__ == "__main__":
    main()
