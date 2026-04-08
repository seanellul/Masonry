---
title: Buildings
tags: [game, system, build-menu]
status: draft
last_updated: 2026-04-07
sources: [content/db/ingnomia.db.sql, src/gui/ui/ui_gamehud.cpp]
---

# Buildings

Every structure you can place via the **Build** shape tool — workshops, structures, furniture, utility items, and containers. This page is the human-readable companion to the in-game tooltips. The runtime source of truth today is the `$BuildingDesc_<id>` rows in `content/db/ingnomia.db.sql`; a future task can build a script to sync this wiki page → SQL so the wiki becomes the authoring surface.

Hover any item in the build menu to see its name and function description as an in-game tooltip. If a tooltip is missing, it means `$BuildingDesc_<id>` has not been written yet — add a row to the Translation table and mirror it here.

## Workshops

Found under the **Workshops** tab. Each workshop crafts a specific family of items. Gnomes assigned to the relevant profession will accept jobs from these workshops.

### Wood

- **Crude workshop** — Makeshift workshop for basic wooden items (planks, sticks, a simple chair, a workbench, and a chisel). Replace with dedicated workshops as soon as you can.
- **Carpenter** — Crafts wooden furniture, doors, beds, barrels, crates, and other wood-based items from planks.
- **Sawmill** — Cuts raw wood logs into planks. Feeds every other wood workshop.
- **Woodcarver** — Carves wood into statues, statuettes, and puzzle boxes.
- **Furnace** — Refines raw coal. Basic heat source for metalworking.
- **Charcoal Kiln** — Slowly converts wood into charcoal for use as fuel.

### Stone

- **Stonecutter** — Cuts raw stone into blocks used for construction and stone crafts.
- **Stonemason** — Crafts stone furniture, chisels, and pickaxe heads from stone blocks.
- **Stonecarver** — Carves stone into doors, hearths, molds, furnaces, troughs, statues, and pillars.
- **Kiln** — Fires clay into bricks and ceramic statuettes.

### Metal

- **Forge** — Smelts ore into metal bars (iron, bronze, steel, rose gold, silver) and forges anvils.
- **Blacksmith** — Forges iron and steel tools — pickaxe heads, felling-axe heads, hammers, files, and big torches.
- **Metalworker** — Crafts decorative metalwork — statues, statuettes, coins.
- **Weaponsmith** — Forges metal weapons (swords, axes, maces, spears, and more).
- **Armorer** — Forges metal armor (helmets, breastplates, greaves, and shields).

### Food

- **Kitchen** — Prepares cooked meals, baked goods, and preserved food from raw ingredients.
- **Distillery** — Brews beer, wine, and cider from grains, fruits, and honey.
- **Butcher** — Processes animal carcasses into meat, leather, and bones.
- **Fishery** — Catches fish from adjacent water tiles and processes them into food.
- **Windmill** — Grinds grain into flour using wind power.

### Crafts

- **Loom** — Weaves cloth bolts from plant fibers and wool.
- **Tailor** — Sews bags, sacks, bandages, mattresses, and padding from cloth bolts.
- **Dyer** — Produces dyes from plants and applies them to dye furniture and cloth.
- **Leatherworker** — Tans hides and crafts leather armor, bags, and accessories.
- **Gemcutter** — Cuts raw gems into finished gems and produces flint pickaxe heads.
- **Jeweler** — Crafts rings and necklaces, optionally set with gems.
- **Bonecarver** — Carves bone into tools, ornaments, and small items.
- **Glass Furnace** — Melts sand into molten glass for the glass maker to shape.
- **Glass Maker** — Shapes molten glass into windows, bottles, and decorative items.

### Mechanics

- **Engineer** — Designs and assembles mechanical components and devices.
- **Machine Shop** — Assembles complex machinery from metal parts and mechanisms.
- **Automaton Maker** — Builds and repairs automaton workers.

### Misc

- **Melee Training** — Training ground where gnomes practice melee combat on a training dummy.
- **Market Stall** — Sells surplus goods to visiting traders.
- **Prospector** — Analyses ore samples and identifies mineral veins in the surrounding stone.
- **Waste Disposal** — Safely destroys unwanted items and corpses.

## Containers

Found under the **Containers** tab. Used inside stockpiles to group and protect items, or carried to speed up hauling.

- **Crate** — General-purpose storage. Most versatile container for stockpiles.
- **Barrel** — Stores liquids and brewed goods. Required by breweries.
- **Bucket** — Small liquid carrier used by gnomes for fetching water.
- **Bag** — Stores small items, plant matter, and seeds.
- **Sack** — Stores grain, flour, and other bulk dry goods.
- **Wheelbarrow** — Lets a gnome haul multiple items at once — speeds up transport jobs.
- **Chest** — Secure storage for small, high-value items.

## Utility

Found under the **Utility** tab. Functional objects that support your settlement rather than being crafting workshops.

- **Door** — Opens and closes to control access. Blocks enemies but not allies.
- **Torch** — Small light source. Illuminates a few surrounding tiles.
- **Wall Torch** — Light source mounted on a wall. Does not occupy a floor tile.
- **Big Torch** — Large light source with a wider illumination radius than a regular torch.
- **Brazier** — Permanent fire pit. Provides light and a bit of warmth.
- **Alarm Bell** — Rung during emergencies to summon gnomes to a defensive position.

## Structures

Found under the **Structures** tab. Per-material variants share the same category-level description. Pick the material (wood / soil / stone / metal) when placing.

- **Wall** — Impassable wall. Blocks movement and line of sight. Supports floors and ceilings above.
- **Floor** — Walkable floor surface. Required to finish enclosed rooms and to hold furniture above dug ground.
- **Stairs** — Connects different Z-levels. Gnomes use them to move up and down.
- **Ramp** — Gentle incline that lets gnomes (and carts) cross a one-level height difference.
- **Fence** — Low barrier. Blocks animals and keeps pastures enclosed.

## Furniture

*(Coverage pending — hover tooltips fall back to the item name only until `$BuildingDesc_<id>` rows are added for each furniture piece. See the inbox for a follow-up expansion task.)*

## See also

- [[roadmap]]
- [[known-issues]]
- Build menu UI: `src/gui/ui/ui_gamehud.cpp`
- Tooltip lookup helper: `buildingTooltipDesc()` in `ui_gamehud.cpp`
- SQL source: `content/db/ingnomia.db.sql` (`$BuildingDesc_*` rows near the end of the file)
