# Credits

## Original Project

Masonry is a fork of **Ingnomia**, originally created by Ralph Schurade and the Ingnomia Team.

- **Original repository**: https://github.com/rschurade/Ingnomia
- **Original license**: GNU AGPL v3
- **Original authors**: Ralph Schurade and contributors (2017-2020)

Ingnomia was itself an independent remake of [Gnomoria](https://store.steampowered.com/app/224500/Gnomoria/) by Robotronic Games, from which it was permitted to borrow certain assets.

## Masonry Changes

Masonry has been substantially rewritten from the original Ingnomia codebase. Major changes include:

- Complete GUI replacement (Noesis/XAML removed, replaced with Dear ImGui)
- macOS port (OpenGL 4.3 downgraded to 4.1 core profile, TBO-based rendering)
- Post-processing shader pipeline (bloom, fog, seasonal color grading)
- Fog of war system for underground exploration
- New test infrastructure with MCP-based automated testing
- Numerous gameplay, UI, and performance improvements

## Third-Party Libraries

- [Qt 5](https://www.qt.io/) — Application framework (LGPL)
- [Dear ImGui](https://github.com/ocornut/imgui) — Immediate mode GUI (MIT)
- [OpenAL Soft](https://openal-soft.org/) — Audio (LGPL)
- [FastNoise](https://github.com/Auburn/FastNoise) — Terrain generation (MIT)
- [exprtk](https://github.com/ArashPartow/exprtk) — Expression parsing (MIT)

## Fonts

- **HermeneusOne** — Title font (SIL Open Font License)
- **PT Root UI** — UI font (Paratype Free Font License)
- **8-bit Operator+** — Pixel font (SIL Open Font License)
