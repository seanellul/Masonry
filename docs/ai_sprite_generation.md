# AI Sprite Generation — Knowledge Base

> Reference doc for generating game-ready pixel art sprites using AI tools.
> Built from community research, independent evaluation, and hands-on experimentation with Masonry's art style.
>
> Last updated: 2026-04-01

---

## 1. Overview & State of the Art

AI image generation has reached a point where it can produce usable game sprites — but with important caveats. The field splits into two distinct problems:

**Static tile/object generation** — Generating individual sprites (terrain tiles, furniture, items). This is the more mature area. Tools like Scenario AI and PixelLab can produce consistent tilesets, especially when trained on reference art.

**Animated sprite generation** — Generating multi-frame animation strips (walk cycles, attack sequences). This is harder. Frame-by-frame generation produces inconsistent results; the emerging best practice is generating full strips in a single pass, anchored to a "seed frame."

### Why Isometric Is Harder

Most AI sprite tutorials and tools target side-view 2D characters. Isometric art is harder for AI because:
- Without explicit "isometric" + "2:1 perspective" in prompts, models default to side-view
- Tile edge alignment matters — AI doesn't naturally produce tiles that seamlessly connect
- The perspective must be consistent across every asset in the set
- Small pixel art (16x16–32x32) is at the limit of what current models handle well; results improve at 64x64+

---

## 2. Tools & Platforms

### Tier 1 — High Relevance to Masonry

#### PixelLab
- **What**: Dedicated AI pixel art generator with native isometric support
- **URL**: https://www.pixellab.ai/
- **Key features**: 4/8-directional character variants, text-to-pixel-art, AI animation (text or skeleton-based), style consistency via reference images, map & tileset generation, AI inpainting
- **Pricing**: Free tier, $5/mo (Hobby), $8/mo (Creator), $24/mo (Studio)
- **Strengths**: Purpose-built for game pixel art; isometric is a first-class use case; can generate directional variants automatically
- **Limitations**: Results noticeably weaker at 16x16 and below; no built-in manual pixel editor
- **Masonry fit**: Strong candidate for generating new furniture, workshop, and creature sprites in isometric style

#### Scenario AI
- **What**: Custom model training platform — train on your art, generate variations
- **URL**: https://www.scenario.com/
- **Key features**: Train Flux Kontext LoRA on 10-20 reference images, inpainting workflow, Edit with Prompts (Gemini 2.5 / Flux Kontext / GPT-Image), sketch-to-tile, batch generation
- **Pricing**: Free tier available, paid plans for more training/generation
- **Strengths**: Train directly on our tilesheets for style-locked output; batch workflow produces uniform tilesets; inpainting allows iterative refinement
- **Limitations**: Training takes 30-45 min; requires curating clean reference sets; still needs manual touch-up for tile seams
- **Masonry fit**: Best approach for generating tiles that match our existing grey stone aesthetic. Train on 15-20 tiles from `terrain.png` / `furniture.png`, then generate new variations

**Scenario training workflow:**
1. Curate 10-20 reference tiles (consistent perspective, lighting, edge logic)
2. Upload as before/after pairs with instruction: "Turn this into an isometric tile matching this style"
3. Train (30-45 min). Add 2-4 test images to monitor quality across epochs
4. Generate with same instruction + new inputs → batch produces uniform tiles

#### SpriteCook
- **What**: AI sprite generator with MCP integration for coding agents
- **URL**: https://www.spritecook.ai/
- **Key features**: Text-to-sprite (pixel art or HD), sprite animation, MCP tools at `api.spritecook.ai/mcp/`, style reference system (generate hero asset, use its ID as reference for subsequent assets)
- **Pricing**: Credit-based system
- **Strengths**: Works directly inside Claude Code / Cursor / VS Code via MCP; style consistency through reference IDs; generates in under 30 seconds
- **Limitations**: Credit costs; less control over exact pixel placement
- **Masonry fit**: Lowest-friction option — could integrate into our existing MCP setup alongside nano-banana. Good for rapid prototyping of new asset concepts

### Tier 2 — Medium Relevance

#### Pixel Engine
- **What**: Custom-trained AI model specifically for pixel art animation
- **URL**: https://pixelengine.ai/
- **Key features**: Upload sprite (16-256px height), describe motion in text, get looping animation. Built-in Piskel-based pixel editor. API access (Python/JS)
- **Pricing**: Free (100 credits/mo), $12/mo (2K credits), $20/mo (4K), $60/mo (12K)
- **Strengths**: Purpose-built for pixel art animation (not adapted from a general model); integrated editor; API for pipeline automation
- **Limitations**: Character animation focus — less useful for terrain/furniture tiles; ~90 second generation time
- **Masonry fit**: Good for animating gnome sprites or creature walk cycles if we create new characters

#### Nano Banana 2 (Gemini)
- **What**: Gemini-based image generation — we already have MCP access
- **Key features**: `generate_image`, `edit_image`, `continue_editing` via MCP
- **Strengths**: Already integrated in our workflow; good for logos, icons, concept exploration
- **Limitations**: Less control for precise pixel art; struggles with structural edits; model name needed patching (`gemini-2.5-flash-image` not `-preview`)
- **Masonry fit**: Logo/branding work (proven). For tile generation, other tools offer more control. Use as quick concept generator
- **Our learnings**: Fresh `generate_image` > `continue_editing` for structural changes. Explicit "no anti-aliasing, visible pixels, pure black background" needed

#### Godot Pixel Renderer
- **What**: Open-source 3D→pixel art converter built on Godot 4.4
- **URL**: https://bukkbeek.itch.io/pixel-renderer / https://github.com/bukkbeek/GodotPixelRenderer
- **Key features**: Real-time 3D to pixel art with customizable pixelation (8-800px), color quantization (2-32 colors), edge detection, dithering, outlines, batch multi-angle rendering
- **Pricing**: Free (open source)
- **Strengths**: Full artistic control; deterministic output; can batch-render all angles; Godot-native
- **Limitations**: Requires 3D models as input; learning curve for Godot setup
- **Masonry fit**: If we ever create 3D models for new buildings/objects, this renders them as pixel art isometric sprites matching our style

**Godot isometric rendering setup:**
- Camera: orthographic, rotation 30/45/0 (standard isometric)
- Viewport: transparent BG, "Always" update mode
- Export: GDScript captures frames, can batch-rotate for multi-angle sprites

#### tilemapgen
- **What**: Open-source Stable Diffusion pipeline for isometric dungeon tiles
- **URL**: https://github.com/charmed-ai/tilemapgen
- **Key features**: Swatch generation → tile rendering → SD depth2image variations → tilemap compilation
- **Pricing**: Free (MIT license). Requires NVIDIA GPU (tested on RTX 3090)
- **Strengths**: Full pipeline from texture to tilemap; uses depth2image for variations from a rendered base; open source
- **Limitations**: Requires local GPU; Ubuntu-focused; Python/PyTorch3D dependencies
- **Masonry fit**: Could adapt the depth2image variation approach to generate terrain tile variants from our existing tiles

#### ComfyUI + Pixel Art LoRAs
- **What**: Local Stable Diffusion pipeline with pixel art specialization
- **Key LoRAs**: Pixel Art XL (civitai.com/models/120096), Microverse LoRA
- **Key features**: Node-based workflow; image pixelate node for grid-consistent output; full local control
- **Pricing**: Free (requires local GPU)
- **Strengths**: No API costs; complete control over generation; pixel art LoRA + pixelate node produces grid-consistent results
- **Limitations**: Setup complexity; requires decent GPU; learning curve
- **Masonry fit**: Good long-term option for batch-generating tile variants locally

### Tier 3 — Situational / Post-Processing

#### GPT Image 1.5 (OpenAI Edit API)
- **What**: OpenAI's image generation/editing model
- **Key features**: Edit API with reference canvas for multi-frame strips; strong instruction-following
- **Strengths**: Best-in-class for seed-frame strip generation (character animations); good at maintaining consistency within a single strip
- **Limitations**: Less suited for isometric tiles; API costs; requires canvas preparation scripts
- **Masonry fit**: If we need character animation strips, the seed-frame workflow is proven

#### Kling 3.0
- **What**: Video generation from static images (by Kuaishou)
- **Strengths**: Creates smooth animation videos from single sprites; good for complex motions
- **Limitations**: Output is video (needs frame extraction + normalization); character-focused
- **Masonry fit**: Low — most of our sprites are static tiles, not animated characters

#### BiRefNet (Alpha Matting)
- **What**: Neural network for high-resolution image segmentation / alpha matting
- **Paper**: Zheng et al., "Bilateral Reference for High-Resolution Dichotomous Image Segmentation" (CAAI AIR 2024, arXiv:2401.03407)
- **Key technique**: `final_alpha = max(BiRefNet_alpha, brightness_alpha)` — combines ML segmentation with luminance-based alpha for clean sprite extraction
- **Strengths**: Produces artifact-free transparency; works with dynamic lighting; HR-matting variant is best
- **Limitations**: Requires NVIDIA GPU for local inference (or Colab free tier)
- **Masonry fit**: Utility tool — apply after any generation pipeline to get clean transparent sprites

#### TRELLIS 2 / Hunyuan3D-2.1 / Tripo
- **What**: Photo→3D model converters (via Krea.ai)
- **Strengths**: Can convert real building photos to 3D models, then render as isometric tiles
- **Limitations**: Multi-step pipeline (photo→3D→render→pixelate); quality varies; complex workflow
- **Masonry fit**: Low priority — the Godot Pixel Renderer is more direct if we have 3D models

### Other Notable Tools

| Tool | What | Notes |
|------|------|-------|
| **Sprite AI** (sprite-ai.art) | AI generation at game-ready sizes (16x16–128x128), built-in editor | $5-24/mo. Good for quick item sprites |
| **God Mode AI** (godmodeai.co) | 8-direction walking/running/combat sprites | Focused on character animations |
| **AutoSprite** (autosprite.io) | AI spritesheet generator | Animation-focused |
| **Rosebud AI** (lab.rosebud.ai) | Online animated sprite maker | Browser-based |
| **Retro Diffusion** | Sprite sheets with frame sequences, 16x16–512x512, style presets | Walk cycles, idle, VFX loops |
| **SEELE** (seeles.ai) | AI asset pipeline, <10 sec per sprite | Focuses on speed |
| **Aseprite** | Manual pixel editor (industry standard), $20 one-time | For hand-refining AI output |

---

## 3. Workflows

### A. Train-on-Your-Art (Recommended First Try)

**Tools**: Scenario AI (or ComfyUI + custom LoRA)
**Best for**: Generating new tiles/objects that match existing Masonry art

1. Curate 15-20 clean references from our tilesheets (e.g., stone cubes from `terrain.png`, furniture from `furniture.png`)
2. Train a custom model (Scenario: upload pairs + instruction; ComfyUI: train LoRA)
3. Generate new tiles with prompts specifying desired objects
4. Post-process: clean up tile edges, verify grid alignment, ensure transparent backgrounds
5. Import into tilesheet at correct grid positions

**Why this is #1**: Produces output that inherently matches our style. Other approaches require more prompt engineering to match.

### B. Seed-Frame Strip Generation

**Tools**: GPT Image 1.5 or Nano Banana 2
**Best for**: Character/creature animation strips

1. Pick one approved in-game sprite frame as the "seed" (locks palette, proportions, silhouette)
2. Upscale with nearest-neighbor, place into a larger transparent canvas with reserved frame slots (e.g., 1024x1024 with 4x 256x256 slots)
3. Prompt for full animation strip in one request (NOT frame-by-frame)
4. Normalize output: detect sprites, compute one shared global scale, pad into fixed-size frames (e.g., 32x32)
5. Lock frame 1 back to the exact shipped sprite

**Key prompt structure** (from GPT Image 1.5 workflow):
```
Edit the provided transparent reference-canvas image into a single horizontal
[N]-frame [animation type] spritesheet. The existing sprite in the leftmost
slot is the exact shipped [description] starting frame and must remain...
[describe action sequence]...
Style: authentic 16-bit pixel art, crisp pixel clusters, stepped shading,
restrained palette, production game asset, not concept art.
Constraints: no [list unwanted elements], keep wide transparent empty space
outside the frame slots.
```

### C. 3D→Pixel Art Pipeline

**Tools**: Godot Pixel Renderer or Blender + post-processing
**Best for**: New buildings, large structures, complex objects

1. Model the object in 3D (or use AI to generate a 3D model from photo/description)
2. Set up isometric camera (orthographic, 30/45/0 rotation)
3. Render with pixel art shader (Godot Pixel Renderer: pixelation + color quantization + edge detection)
4. Batch-export multiple angles if needed
5. Post-process in Aseprite for final polish

### D. MCP Agent Pipeline

**Tools**: SpriteCook + Claude Code (or Cursor)
**Best for**: Rapid prototyping during development

1. Install SpriteCook MCP (automated installer detects editors)
2. Generate a "hero asset" first — this becomes the style anchor
3. Use the hero asset's ID as `style_reference` for all subsequent generations
4. Agent generates sprites inline while coding, maintaining visual consistency
5. Review and refine before adding to tilesheets

### E. ComfyUI Local Pipeline

**Tools**: ComfyUI + Stable Diffusion XL + Pixel Art XL LoRA
**Best for**: Batch generation without API costs

1. Install ComfyUI + download Pixel Art XL LoRA and Microverse LoRA
2. Build workflow: text prompt → SDXL + LoRA → image pixelate node (for grid consistency)
3. Configure pixelation size to match target sprite dimensions
4. Generate batches with consistent prompts
5. Post-process for transparent backgrounds and grid alignment

### F. Video-to-Spritesheet

**Tools**: Kling 3.0 + frame extraction + BiRefNet
**Best for**: Complex character animations (attack combos, hurt reactions)

1. Generate animation video from a static sprite using Kling 3.0
2. Extract frames at lossless quality
3. Run BiRefNet for clean alpha extraction: `final_alpha = max(BiRefNet_alpha, brightness_alpha)`
4. Normalize into fixed-size frames with shared global scale
5. Optionally compute depth maps (MiDaS) for PBR/lighting effects

---

## 4. Masonry-Specific Analysis

### Our Art Style (from tilesheet deep-dive)

**Terrain** (`terrain.png`):
- Core material is **grey stone** — cool greys, blue-greys, dark charcoal
- Isometric cubes with visible face shading (top light, left mid, right dark)
- Mineral ores add color pops: gold (#FFD700-ish), orange, green gems, blue crystals
- Some transparency effects (ice, glass)

**Furniture & Workshops** (`furniture.png`, `workshops.png`):
- Grey stone + brown wood construction
- Muted, functional aesthetic — no flashy colors
- Barrels, tables, furnaces, anvils, beds, storage containers
- Consistent lighting direction across all objects

**Mechanisms** (`traps_mechanism.png`):
- Almost entirely greyscale
- Gears, levers, platforms, gates, bridges, mechanical components
- Clean, precise pixel work

**Characters** (`gnomes.png`, `mobs.png`):
- Composited body parts system (heads, torsos, helmets, boots, weapons)
- Very small sprites — consistency across pieces is critical
- Gnome icon (`content/icon.png`) is higher-res than in-game sprites
- Mobs: golems, skeletons, slimes in green/grey/teal

**Nature** (`multitrees.png`, `seasonalgrass.png`, `plants.png`):
- Rich greens with seasonal variation (green → gold → white/snow)
- Isometric diamond tiles for grass overlays
- Trees are multi-tile composites

**Animals** (`animals.png`, `multicreatures.png`):
- Small isometric creatures, limited palette per species
- Farm animals (pigs, sheep, chickens) + wild (bears, wolves, lions)
- Multi-tile large creatures (mammoths, tigers)

### Key Constraints for AI Generation

1. **Perspective**: Standard isometric (2:1 ratio, ~30 degree angle)
2. **Grid**: Sprites must align to the tilesheet grid — transparent backgrounds, consistent bounding boxes
3. **Palette**: Grey stone dominant. Limited warm tones (wood, leather). No vibrant/saturated colors except specific ores
4. **Scale**: Most tiles are small (32x32 base). AI works better at 64x64+, so generate larger and downscale with nearest-neighbor
5. **Style**: No anti-aliasing, visible pixels, stepped shading, no gradients (except subtle ambient)
6. **Transparency**: All sprites on transparent background — BiRefNet can help clean up AI-generated backgrounds

---

## 5. Lessons Learned

### From Community Research

- **Full strip > frame-by-frame** for character animation consistency. Image models resist animating; generating the whole strip at once produces more coherent motion
- **Seed frames are essential** — anchor to a shipped sprite to lock palette, proportions, and silhouette. The seed frame matters more than the prompt
- **One global scale for normalization** — don't scale each frame independently, or tall poses shrink the whole character. Compute a shared scale from the largest frame
- **Nearest-neighbor upscaling** preserves pixel art crispness. Never use bilinear/bicubic for pixel art
- **8-20 clean references** is enough for custom model training (Scenario AI). Quality > quantity — consistent perspective, lighting, and edge logic
- **"Isometric" in prompts is mandatory** — without it, AI defaults to side-view. Add "2:1 perspective" for precision
- **Lock frame 1 to the shipped sprite** — replace the AI-generated first frame with the exact production sprite for seamless animation blending

### From Our Own Gemini/Nano Banana Experience

- **Fresh `generate_image` > `continue_editing`** for structural changes (removing elements, repositioning)
- **Explicit negative instructions needed**: "no anti-aliasing", "visible pixels", "pure black background" (otherwise white bars appear)
- **"NOT isometric, NOT angled"** needed for front-facing text renders
- **Parallel generation** works well for exploring variations — run multiple prompts simultaneously
- **AI struggles with text in pixel art** — letter forms often break. Better to use the model for visual elements and add text separately

---

## 6. Recommended Experiments

Prioritized for Masonry's needs, from lowest-friction to highest-potential:

### Experiment 1: SpriteCook MCP Integration
- **Effort**: Low (install MCP, test with a few prompts)
- **Goal**: Test if SpriteCook can generate furniture/item sprites matching our style
- **How**: Add SpriteCook MCP to `.mcp.json`, generate a hero asset from a terrain tile reference, then generate variations

### Experiment 2: PixelLab Isometric Generator
- **Effort**: Low-Medium (sign up, upload references, test generation)
- **Goal**: Test PixelLab's isometric tile generation with our grey stone style as reference
- **How**: Upload 5-10 terrain tiles as style reference, prompt for new tile types

### Experiment 3: Scenario AI Custom Model
- **Effort**: Medium (curate dataset, train, iterate)
- **Goal**: Train a model that reliably generates tiles matching our exact art style
- **How**: Extract 15-20 tiles from `terrain.png` + `furniture.png`, train Flux Kontext LoRA, batch-generate new tiles
- **Expected best results** but highest setup cost

### Experiment 4: Godot Pixel Renderer
- **Effort**: Medium-High (install Godot 4.4, set up scene, configure shaders)
- **Goal**: Test 3D→pixel art pipeline for new building/structure types
- **How**: Model a simple stone structure in Godot, render with pixel art shader at isometric angle, compare to existing tiles

---

## References

### Articles & Threads (User-Collected)
- GPT 5.4 + Image 1.5 sprite animation workflow (seed-frame strip method)
- TechHalla (@techhalla) — Nano Banana 2 + Kling 3.0 + custom spritesheet app
- BiRefNet alpha matting for game sprites (Day 50 webGL engine series)
- Justine Moore (@venturetwins) — Photo→3D→isometric pipeline with Cursor
- Pixel Engine vs Nano Banana 2 comparison

### Tools & Platforms
- PixelLab: https://www.pixellab.ai/
- Scenario AI: https://www.scenario.com/
- SpriteCook: https://www.spritecook.ai/
- Pixel Engine: https://pixelengine.ai/
- Godot Pixel Renderer: https://github.com/bukkbeek/GodotPixelRenderer
- tilemapgen: https://github.com/charmed-ai/tilemapgen
- Sprite AI: https://www.sprite-ai.art/
- ComfyUI Pixel Art XL LoRA: https://civitai.com/models/120096/pixel-art-xl
- Aseprite: https://www.aseprite.org/

### Guides
- Scenario: Build Isometric Tiles with AI — https://www.scenario.com/blog/build-isometric-game-tiles-with-ai
- Scenario: Flux Kontext LoRA for Isometric Tiles — https://www.scenario.com/blog/flux-kontext-lora-isometric-building-tiles
- Kenney: Rendering Isometric Sprites in Godot — https://kenney.nl/knowledge-base/learning/rendering-isometric-sprites-using-godot
- ComfyUI Pixel Art Workflow — https://www.kokutech.com/blog/gamedev/tips/art/pixel-art-generation-with-comfyui
- BiRefNet paper — arXiv:2401.03407
