#!/usr/bin/env python3
"""
Post-process AI restyled sprites back into 32×36 pixel-art form that fits
the engine's constraints. For each output:

  1. Downsample to 32×36 with LANCZOS (smooth), then quantize to a limited
     palette (median-cut 8 colors).
  2. Apply the original alpha mask: any pixel outside the original silhouette
     is forced to transparent. This eliminates shape drift and guarantees the
     new sprite is drop-in compatible with the engine.
  3. Save a before/after/mask-locked triptych per sprite for evaluation.
"""
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parent.parent
DIR = REPO / "tools" / "sprite_pipeline" / "cozy_test"

# (label, AI output filename)
JOBS = [
    ("gnome_head", "edited-2026-04-07T14-49-03-774Z-6s6mvk.png"),
    ("workbench",  "edited-2026-04-07T14-49-13-911Z-z5i2be.png"),
    ("rabbit",     "edited-2026-04-07T14-49-22-753Z-ypy04m.png"),
]

W, H = 32, 36
ZOOM = 12
PAD = 12


def downsample(img: Image.Image) -> Image.Image:
    return img.convert("RGBA").resize((W, H), Image.LANCZOS)


def quantize(img: Image.Image, n: int = 8) -> Image.Image:
    # Separate alpha to preserve transparency, quantize RGB only.
    r, g, b, a = img.split()
    rgb = Image.merge("RGB", (r, g, b))
    q = rgb.quantize(colors=n, dither=Image.Dither.NONE).convert("RGB")
    qr, qg, qb = q.split()
    return Image.merge("RGBA", (qr, qg, qb, a))


def mask_lock(img: Image.Image, mask: Image.Image) -> Image.Image:
    out = img.copy()
    pm = mask.load()
    po = out.load()
    for y in range(H):
        for x in range(W):
            if pm[x, y] < 16:
                po[x, y] = (0, 0, 0, 0)
    return out


def panel(original_z, ai_z, final_z, label, font):
    from PIL import ImageDraw
    cw = W * ZOOM
    ch = H * ZOOM
    w = PAD + 3 * (cw + PAD)
    h = 30 + ch + 22
    img = Image.new("RGBA", (w, h), (24, 24, 28, 255))
    draw = ImageDraw.Draw(img)
    draw.text((PAD + 6, 6), label, fill=(220, 220, 220, 255), font=font)
    for i, (name, im) in enumerate([("orig", original_z), ("ai", ai_z), ("final", final_z)]):
        x = PAD + i * (cw + PAD)
        img.paste(im, (x, 26), im)
        draw.text((x + 4, 26 + ch + 2), name, fill=(160, 200, 255, 255), font=font)
    return img


def main():
    from PIL import ImageDraw, ImageFont
    try:
        font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 14)
    except OSError:
        font = ImageFont.load_default()

    panels = []
    for label, fname in JOBS:
        ai_path = REPO / "generated_imgs" / fname
        mask_path = DIR / f"{label}_mask.png"
        orig_path = DIR / f"{label}_orig_native.png"
        if not ai_path.exists():
            print(f"MISSING: {ai_path}")
            continue
        ai = Image.open(ai_path).convert("RGBA")
        mask = Image.open(mask_path).convert("L")
        orig = Image.open(orig_path).convert("RGBA")

        # Demagenta the original for display
        po = orig.load()
        for y in range(H):
            for x in range(W):
                r, g, b, a = po[x, y]
                if r > 240 and b > 240 and g < 16:
                    po[x, y] = (0, 0, 0, 0)

        down = downsample(ai)
        quant = quantize(down, n=8)
        final = mask_lock(quant, mask)
        final.save(DIR / f"{label}_final.png")

        # Zoom for triptych
        zz = lambda im: im.resize((W * ZOOM, H * ZOOM), Image.NEAREST)
        panels.append(panel(zz(orig), zz(down), zz(final), label, font))

    if not panels:
        return
    total_h = sum(p.size[1] for p in panels) + (len(panels) - 1) * 8
    total_w = max(p.size[0] for p in panels)
    out = Image.new("RGBA", (total_w, total_h), (18, 18, 22, 255))
    y = 0
    for p in panels:
        out.paste(p, (0, y), p)
        y += p.size[1] + 8

    out_path = DIR / "cozy_triptych.png"
    out.save(out_path)
    print(f"Wrote {out_path}  ({out.size})")


if __name__ == "__main__":
    main()
