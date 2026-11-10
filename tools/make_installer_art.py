#!/usr/bin/env python3
"""Draws the installers' artwork and the Windows icon from the app icon's SVG,
in the style of Lithium Drums' installers. Run it after changing the logo; its
output is committed, so builds don't need it.

    python3 tools/make_installer_art.py

Needs Pillow. Writes:
    resources/icons/<app id>.png                 the app icon (1024 px; compiled into the app)
    resources/icons/LithiumStudio.ico            the Windows icon (16 to 256 px)
    packaging/windows/installer-side[-2x].bmp    the wizard's side panel
    packaging/windows/installer-small[-2x].bmp   the wizard's top-right picture
    packaging/macos/resources/background.png     the macOS Installer's background
    packaging/linux/icons/icon-<size>.png        the app icon for the Linux installer's menu entry

The icon SVG is a gradient tile (a rounded square), its border (a slight blue
edge, 3 units wide as on Lithium Drums' icon) and groups of rounded cells;
this reads just those shapes, so it needs no SVG renderer.
"""

import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SVG = ROOT / "resources/icons/io.github.thecalamityjoe87.LithiumStudio.svg"
FONTS = ROOT / "resources/fonts"

BACKGROUND = (0x0E, 0x13, 0x18)      # the Lithium palette's background
RAISED_HIGH = (0x1D, 0x28, 0x33)
TEXT = (0xE6, 0xED, 0xF2)
MUTED = (0x93, 0xA3, 0xB1)
COMPANY = "Calamity Audio"
SUPERSAMPLE = 4


def hex_colour(h):
    h = h.lstrip("#")
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4)) + (255,)


def read_svg():
    text = SVG.read_text()
    stops = [hex_colour(c) for c in re.findall(r'stop-color="(#[0-9a-fA-F]{6})"', text)]
    tx, tw, trx = map(float, re.search(r'<rect x="([\d.]+)" y="[\d.]+" width="([\d.]+)" height="[\d.]+" rx="([\d.]+)" '
                                       r'fill="url\(#tile\)"/>', text).groups())
    tile = ((tx, tx, tx + tw, tx + tw), trx)
    m = re.search(r'<rect x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" height="[\d.]+" rx="([\d.]+)" '
                  r'fill="none" stroke="(#[0-9a-fA-F]{6})" stroke-width="([\d.]+)"/>', text)
    x, y, w, rx = map(float, m.group(1, 2, 3, 4))
    sw = float(m.group(6))
    # An SVG stroke is centred on the path; Pillow's outline is drawn inwards
    # from its box, so the box is the rectangle grown by half the stroke.
    border = (hex_colour(m.group(5)), (x - sw / 2, y - sw / 2, x + w + sw / 2, y + w + sw / 2), rx + sw / 2, sw)
    cells = []
    for fill, body in re.findall(r'<g fill="(#[0-9a-fA-F]{6})">(.*?)</g>', text, re.S):
        for x, y, w, h, rx in re.findall(r'<rect x="([\d.]+)" y="([\d.]+)" width="([\d.]+)" '
                                         r'height="([\d.]+)" rx="([\d.]+)"/>', body):
            cells.append((hex_colour(fill), *map(float, (x, y, w, h, rx))))
    return stops, tile, border, cells


def vertical_gradient(size, top, bottom, y0=0.0, y1=None):
    w, h = size
    y1 = h if y1 is None else y1
    img = Image.new("RGBA", size)
    draw = ImageDraw.Draw(img)
    for y in range(h):
        f = min(1.0, max(0.0, (y - y0) / max(1.0, y1 - y0)))
        draw.line([(0, y), (w, y)], fill=tuple(round(a + (b - a) * f) for a, b in zip(top, bottom)))
    return img


def render_icon(px, with_tile=True):
    """The app icon at px x px (its 256-unit SVG scaled), transparent around the tile."""
    stops, (tile_box, tile_radius), border, cells = read_svg()
    s = px * SUPERSAMPLE
    k = s / 256.0
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    if with_tile:
        mask = Image.new("L", (s, s), 0)
        box = [c * k for c in tile_box]
        ImageDraw.Draw(mask).rounded_rectangle(box, radius=tile_radius * k, fill=255)
        img.paste(vertical_gradient((s, s), stops[0], stops[1], box[1], box[3]), (0, 0), mask)
        colour, box, radius, width = border
        ImageDraw.Draw(img).rounded_rectangle([c * k for c in box], radius=radius * k,
                                              outline=colour, width=max(1, round(width * k)))
    draw = ImageDraw.Draw(img)
    for fill, x, y, w, h, rx in cells:
        draw.rounded_rectangle([x * k, y * k, (x + w) * k, (y + h) * k], radius=rx * k, fill=fill)
    return img.resize((px, px), Image.LANCZOS)


def render_mark(width):
    """Just the lit cells (the waveform), cropped to them, width px wide."""
    big = render_icon(1024, with_tile=False)
    mark = big.crop(big.getbbox())
    return mark.resize((width, round(mark.height * width / mark.width)), Image.LANCZOS)


def tracked_width(font, text, tracking):
    return sum(font.getlength(c) for c in text) + tracking * font.size * (len(text) - 1)


def draw_tracked(draw, centre_x, y, text, font, tracking, fill):
    x = centre_x - tracked_width(font, text, tracking) / 2
    for c in text:
        draw.text((x, y), c, font=font, fill=fill)
        x += font.getlength(c) + tracking * font.size


def side_panel(scale):
    """The wizard's side panel: the mark, the wordmark on two lines, the company."""
    w, h = 164 * scale, 314 * scale
    img = vertical_gradient((w, h), RAISED_HIGH + (255,), BACKGROUND + (255,))
    mark = render_mark(round(w * 0.62))
    img.alpha_composite(mark, ((w - mark.width) // 2, round(h * 0.36 - mark.height / 2)))

    draw = ImageDraw.Draw(img)
    bold = ImageFont.truetype(str(FONTS / "BarlowCondensed-Bold.ttf"), 23 * scale)
    semi = ImageFont.truetype(str(FONTS / "BarlowCondensed-SemiBold.ttf"), 23 * scale)
    draw_tracked(draw, w / 2, h * 0.585, "LITHIUM", bold, 0.18, TEXT)
    draw_tracked(draw, w / 2, h * 0.585 + 29 * scale, "STUDIO", semi, 0.18, MUTED)

    body = ImageFont.truetype(str(FONTS / "Barlow-Regular.ttf"), 9.5 * scale)
    draw.text(((w - body.getlength(COMPANY)) / 2, h * 0.9), COMPANY, font=body, fill=MUTED)
    return img


def save_bmp(img, path, background):
    """Inno Setup wants 24-bit BMPs: flatten onto what's behind them."""
    flat = Image.new("RGB", img.size, background)
    flat.paste(img, (0, 0), img)
    flat.save(path, "BMP")


def main():
    windows = ROOT / "packaging/windows"
    macos = ROOT / "packaging/macos/resources"
    macos.mkdir(parents=True, exist_ok=True)

    # The app icon itself (compiled into the app: title bar, About, splash).
    render_icon(1024).save(ROOT / "resources/icons/io.github.thecalamityjoe87.LithiumStudio.png")

    # Windows: one .ico with the sizes Explorer and the taskbar use.
    sizes = [16, 24, 32, 48, 64, 256]
    icons = [render_icon(s) for s in sizes]
    icons[-1].save(ROOT / "resources/icons/LithiumStudio.ico", sizes=[(s, s) for s in sizes], append_images=icons[:-1])

    for scale, suffix in ((1, ""), (2, "-2x")):
        save_bmp(side_panel(scale), windows / f"installer-side{suffix}.bmp", BACKGROUND)
        # The small picture sits on the wizard's white top strip.
        save_bmp(render_icon(55 * scale), windows / f"installer-small{suffix}.bmp", (255, 255, 255))

    # macOS: the app icon at the bottom left of the Installer's window, as on Lithium Drums'.
    background = Image.new("RGBA", (620, 418), (0, 0, 0, 0))
    icon = render_icon(136)
    background.alpha_composite(icon, (42, 418 - 42 - icon.height))
    background.save(macos / "background.png")

    # Linux: the icon at the sizes desktops look for, for the installer to put
    # in the user's icon theme folder.
    linux = ROOT / "packaging/linux/icons"
    linux.mkdir(parents=True, exist_ok=True)
    for s in (16, 24, 32, 48, 64, 128, 256, 512):
        render_icon(s).save(linux / f"icon-{s}.png")

    print("Updated the app icon PNG, resources/icons/LithiumStudio.ico, packaging/windows/installer-*.bmp, "
          "packaging/macos/resources/background.png and packaging/linux/icons/")


if __name__ == "__main__":
    main()
