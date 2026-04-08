#!/usr/bin/env python3
"""
Slice a tilesheet into individual cells (per BaseSprites SourceRectangle entries)
and emit a labelled contact sheet for visual inspection / AI-prompt anchoring.

Usage:
    python3 tools/inventory_tilesheet.py seasonalgrass.png

Outputs into tools/sprite_pipeline/inventory/<sheet_name>/:
    cells/<ID>.png       — one PNG per BaseSprites row
    contact_sheet.png    — labelled grid for visual reference
    manifest.json        — { ID: [x, y, w, h] }
"""
import json
import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parent.parent
DB_SQL = REPO / "content" / "db" / "ingnomia.db.sql"
TILESHEET_DIR = REPO / "content" / "tilesheet"
OUT_ROOT = REPO / "tools" / "sprite_pipeline" / "inventory"

INSERT_RE = re.compile(
    r"INSERT INTO \"BaseSprites\" .*VALUES \('([^']+)','([^']+)','([^']+)'\);"
)


def load_entries(sheet_name: str):
    entries = []
    with DB_SQL.open("r", encoding="utf-8", errors="replace") as f:
        for line in f:
            m = INSERT_RE.search(line)
            if not m:
                continue
            sid, rect, sheet = m.group(1), m.group(2), m.group(3)
            if sheet != sheet_name:
                continue
            parts = rect.split()
            if len(parts) != 4:
                continue
            entries.append((sid, tuple(int(p) for p in parts)))
    return entries


def parse_grid_key(sid: str):
    """For Grass_<row>_<col> -> (row, col); else (None, None)."""
    m = re.match(r"^([A-Za-z]+)_(\d+)_(\d+)$", sid)
    if m:
        return int(m.group(2)), int(m.group(3))
    return None, None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)
    sheet_name = sys.argv[1]
    sheet_path = TILESHEET_DIR / sheet_name
    if not sheet_path.exists():
        print(f"Tilesheet not found: {sheet_path}")
        sys.exit(1)

    entries = load_entries(sheet_name)
    if not entries:
        print(f"No BaseSprites entries reference {sheet_name}")
        sys.exit(1)

    out_dir = OUT_ROOT / sheet_name.replace(".png", "")
    cells_dir = out_dir / "cells"
    cells_dir.mkdir(parents=True, exist_ok=True)

    src = Image.open(sheet_path).convert("RGBA")
    print(f"Loaded {sheet_name}: {src.size}, {len(entries)} cells")

    manifest = {}
    grid_cells = {}  # (row, col) -> (sid, image)
    max_row = 0
    max_col = 0
    cell_w = cell_h = 0

    for sid, (x, y, w, h) in entries:
        crop = src.crop((x, y, x + w, y + h))
        crop.save(cells_dir / f"{sid}.png")
        manifest[sid] = [x, y, w, h]
        row, col = parse_grid_key(sid)
        if row is not None:
            grid_cells[(row, col)] = (sid, crop)
            max_row = max(max_row, row)
            max_col = max(max_col, col)
            cell_w, cell_h = w, h

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2))

    if not grid_cells:
        print("No grid-keyed cells found; skipping contact sheet.")
        return

    # Build a labelled contact sheet: max_row+1 rows x max_col+1 cols, with
    # a header row/column for indices.
    pad = 4
    label_h = 14
    header = 24
    grid_w = max_col + 1
    grid_h = max_row + 1
    img_w = header + grid_w * (cell_w + pad) + pad
    img_h = header + grid_h * (cell_h + label_h + pad) + pad

    contact = Image.new("RGBA", (img_w, img_h), (24, 24, 28, 255))
    draw = ImageDraw.Draw(contact)
    try:
        font = ImageFont.truetype(
            "/System/Library/Fonts/Supplemental/Arial.ttf", 10
        )
        big = ImageFont.truetype(
            "/System/Library/Fonts/Supplemental/Arial Bold.ttf", 12
        )
    except OSError:
        font = ImageFont.load_default()
        big = font

    # Column headers
    for c in range(grid_w):
        x = header + c * (cell_w + pad) + cell_w // 2
        draw.text((x - 6, 4), f"c{c}", fill=(200, 200, 200, 255), font=big)
    # Row headers + cells
    for r in range(grid_h):
        y_top = header + r * (cell_h + label_h + pad)
        draw.text((4, y_top + cell_h // 2 - 6), f"r{r}",
                  fill=(200, 200, 200, 255), font=big)
        for c in range(grid_w):
            cell = grid_cells.get((r, c))
            if not cell:
                continue
            sid, img = cell
            x = header + c * (cell_w + pad)
            contact.paste(img, (x, y_top), img)
            draw.text((x, y_top + cell_h + 1), sid,
                      fill=(160, 200, 255, 255), font=font)

    contact_path = out_dir / "contact_sheet.png"
    contact.save(contact_path)
    print(f"Wrote {contact_path}  ({contact.size})")
    print(f"Wrote {len(manifest)} cells to {cells_dir}")
    print(f"Grid bounds: rows 0..{max_row}, cols 0..{max_col} ({cell_w}x{cell_h} each)")


if __name__ == "__main__":
    main()
