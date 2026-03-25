# Ingnomia Dev Discussion Summary

Extracted from Discord dev/modding channel (2017-11-11 to 2026-02-25)

**2395 messages** across **137 conversation threads** from **70 contributors**

## Key Contributors

| Author | Messages | Role |
|--------|----------|------|
| ext3h | 516 | Major contributor (architecture/optimization) |
| arcnor | 479 | Major contributor (porting/refactoring) |
| .roest | 351 | Original developer |
| cravenwarrior | 197 | Active contributor |
| dinosaure.plz_nofriendrequests | 183 | Active contributor |
| rivenwyrm | 100 | Contributor |
| njoyard | 91 | Contributor |
| mason1920 | 53 | Contributor |
| fris0uman | 48 | Contributor |
| captain_corkscrew | 40 | Community member |
| saint_of_grey | 22 | Community member |
| carlosgrr | 20 | Community member |
| condac | 20 | Community member |
| qtw | 19 | Community member |
| cptvolkow | 17 | Community member |

## Topic Breakdown

| Topic | Threads |
|-------|--------|
| UI / GUI Framework | 50 |
| Game Mechanics | 47 |
| Bug Fixes / Debugging | 46 |
| Git / Version Control | 43 |
| Database / Data Model | 42 |
| Build System / Compilation | 35 |
| Modding / Data Files | 33 |
| Architecture / Refactoring | 32 |
| Rendering / Graphics | 29 |
| General Discussion | 27 |
| Platform / Portability | 24 |
| Performance / Optimization | 20 |
| Save / Load | 17 |
| World Generation | 12 |
| Networking / Multiplayer | 10 |
| Pathfinding / AI | 9 |

---

## Bug Fixes / Debugging

- **2019-08-11** (2 msgs, shapkees, .roest)
  no sorry, haven't touched that issue in a while

- **2020-10-15** (5 msgs, dinosaure.plz_nofriendrequests, .roest, rivenwyrm)
  I might have eventually made some slight changes to your original French message. Possibly, treasures were mentionned, but not 100% sure of that.

- **2020-12-01** (1 msgs, cravenwarrior) `Platform / Portability`
  Update on that issue with the Linux version, NoesisGUI was patched so the next version may work fine without adding template throughout a file

- **2021-01-03** (11 msgs, ext3h, harminoff, .roest) `Git / Version Control`
  Hmm, I'm not seeing errors when using https. The certificate is a letsencrypt cert with a wildcard, so it should be valid. And it's the one I'm getting when accessing the site... :/

- **2021-05-27** (1 msgs, njoyard)
  What's the issue you're getting? Did you follow the instructions in the README?

- **2022-07-22** (1 msgs, arcnor)
  is the sole purpose of `SpriteFactory::getBaseSpriteDryRun()` to fill the `m_randomNumbers` hashmap and then use that to create a unique key of some sort? because it takes a big amount of time when ru

- **2023-11-15** (2 msgs, aidandavis99, itsadraelbaby)
  The game is open-source, which means it will never truly be abandoned; that said, I have stopped my own contributions for some time, now, as I simply do not have room on my plate for it, any longer; I

## Build System / Compilation

- **2020-09-03** (26 msgs, deanec64, .roest, ext3h) `UI / GUI Framework` `Bug Fixes / Debugging` `Platform / Portability`
  well the readme has instructions for that already, compiling from code takes a certain level of knowledge so I'm not sure how much could be added toe README to make it newb proof

- **2020-09-29** (41 msgs, ext3h, .roest, rivenwyrm) `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Architecture / Refactoring` `Git / Version Control`
  No clue why it didn't react to job_prio branch. Maybe it didn't like the branch being created pointing to a commit already, and only triggers when a branch is updated.

- **2020-10-05** (5 msgs, rivenwyrm) `Game Mechanics` `Git / Version Control`
  though I have no idea how the strings will display so my theoretical method might be awful, set myself up with a trial license

- **2020-10-06** (2 msgs, .roest, hughheggen) `Platform / Portability`
  The game compiles on Linux, if you have basic knowledge to install prerequisites, run cmake and compile a program, which you really should have if you're running a linux system, you can get the game r

- **2020-10-07** (88 msgs, djudgya, dinosaure.plz_nofriendrequests, rivenwyrm) `Database / Data Model` `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Git / Version Control`
  Okay, be right back. noesisGUI accepted `a@yopmail.com` as a valid email/username, great.  Okay, that's all for today, the cmdline did not work because cmake was not in the PATH yet and the tilesheet 

- **2020-10-08** (76 msgs, dinosaure.plz_nofriendrequests, .roest, ext3h) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Platform / Portability` `Save / Load` `World Generation` `Git / Version Control`
  <@!280266293490221056> : I've some trouble with the CMake command. At first, there were -DVars missing: ``` cmake .. -DQt5_DIR="C:\Qt\5.14.1\msvc2017_64\lib\cmake\Qt5" -DSTEAM_SDK_ROOT="3rdparty/steam

- **2020-10-18** (1 msgs, ext3h) `Database / Data Model` `Pathfinding / AI` `UI / GUI Framework` `Game Mechanics` `Performance / Optimization`
  <@!593160154376044594> Please do not try to make the buttons or other labels scale with the text. It's really hard to build a layout around such behavior, and especially in table layouts, you are brea

- **2020-10-22** (17 msgs, dinosaure.plz_nofriendrequests, ext3h, .elfu) `UI / GUI Framework` `Game Mechanics` `Architecture / Refactoring`
  Ok, time to stop faking: <@!280266293490221056>  and <@!406873466537508864>  are strongly in favor of a squared-shape button ; I made a modification that unconstrained the max width, constraint that w

- **2020-11-17** (5 msgs, fris0uman, .roest) `Save / Load` `World Generation`
  `SpriteFactory: failed to load  "terrain.png"` nop, that's probably what I'm missing ^^"

- **2020-11-25** (60 msgs, dinosaure.plz_nofriendrequests, carlosgrr, ext3h) `Database / Data Model` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Bug Fixes / Debugging` `Platform / Portability` `Save / Load` `World Generation` `Git / Version Control`
  <@!406873466537508864> since you asked for it just today, like Craven said, they almost build out of the box, I am running it on Arch Linux, like he said you have to change a single file, but another 

- **2020-11-26** (61 msgs, dinosaure.plz_nofriendrequests, carlosgrr, ext3h) `Database / Data Model` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Save / Load` `Networking / Multiplayer` `Git / Version Control`
  ``` <UserControl     x:Class="IngnomiaGUI.Agriculture"     xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"     xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"     xmlns:d="htt

- **2020-11-27** (278 msgs, ext3h, .roest, cravenwarrior) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Save / Load` `World Generation` `Git / Version Control`
  remove the build directory and run ``` cmake .. \     -DQt5_DIR="/home/goren/Qt/5.15.2/gcc_64/lib/cmake" \     -DSTEAM_SDK_ROOT="/home/goren/Downloads/src/steamsdk/sdk" \     -DNOESIS_ROOT="/home/gore

- **2020-11-28** (19 msgs, totensonntag, carlosgrr, ext3h) `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Platform / Portability`
  On the same note about external dependencies, not trying to be the kick in the groin here, but I just checked the licensing page of the noesis engine and their "free - indie" license is "For individua

- **2020-11-29** (17 msgs, ext3h, carlosgrr) `Bug Fixes / Debugging` `Architecture / Refactoring` `Git / Version Control`
  When I try to compile the code using gcc it comes with a lot of warnings (477), what are the odds that if I "fixed" some of the most obvious ones, or commented on some of the issues, someone would be 

- **2020-12-31** (111 msgs, ext3h, .roest, cptvolkow) `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Save / Load` `Networking / Multiplayer` `Git / Version Control`
  Need to figure out which detail about the compute shader `worldupdate_c.glsl` is causing a link failure with the message `SHADER_ID_LINK error has been generated. GLSL link failed for program 4, "": O

- **2021-01-04** (33 msgs, dinosaure.plz_nofriendrequests, ext3h, njoyard) `Database / Data Model` `Rendering / Graphics` `Modding / Data Files` `Game Mechanics` `Bug Fixes / Debugging` `Architecture / Refactoring` `Networking / Multiplayer` `Git / Version Control`
  Saw this, but not in the mood to head-compile this tonight. ```    if ( m_properties.autoHarvestSeed )     {         unsigned int count = Global::inv().itemCount( m_properties.seedItem, seedMaterialID

- **2021-01-08** (29 msgs, ext3h, njoyard, .roest) `Database / Data Model` `Rendering / Graphics` `Game Mechanics` `Bug Fixes / Debugging` `Architecture / Refactoring` `Git / Version Control`
  I'm considering adding this tech tree, basically computed the same way the TechTree game class does (but with workshops).  I'm not really sure because basically each level assumes you have built every

- **2021-01-13** (10 msgs, ext3h, njoyard, .roest) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Git / Version Control`
  Would something like that be useful ? Blue overlays for basesprites and shows basesprite IDs on hover (top right). Generated using a new docs theme. We could also imagine showing Sprites_* entries tha

- **2021-01-21** (30 msgs, ext3h, njoyard) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Networking / Multiplayer` `Git / Version Control`
  Some details you should then also try to understand about Qt containers, how they use copy-on-write-after-fork semantics. Meaning you can safely send a QList over a signal-slot into another thread, *b

- **2021-01-29** (8 msgs, condac, ext3h) `UI / GUI Framework` `Performance / Optimization` `Platform / Portability` `Git / Version Control`
  I am a total noob when it comes to signals and a few things in this code so I have probably done a few things that are not "right way". But it works... at least for me, someone will probably have to t

- **2021-05-26** (2 msgs, .roest, keyarae99) `Game Mechanics`
  Sorry I dont want to be that person but where is jinja2 located because I am trying to build the docs and its been a no go?

- **2021-05-28** (14 msgs, cptvolkow, .roest, nardian13) `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Platform / Portability`
  Hello, I am trying to build this game on Manjaro (Arch-based), but I am running into this issue: ``` Consolidate compiler generated dependencies of target NoesisApp [  0%] Building CXX object 3rdparty

- **2022-06-28** (38 msgs, ext3h, .roest, arcnor) `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Git / Version Control`
  few things that you might or might not like: 1. This first set of changes just removes `QSet` completely, because it was the easiest 2. I've changed the C++ version to C++20, because the STL has impro

- **2022-06-30** (36 msgs, ext3h, .roest, arcnor) `Database / Data Model` `UI / GUI Framework` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring`
  biggest issue with replacing the sigslot stuff is that thread safety is not guaranteed, because the library obviously doesn't know anything about Qt threads, so things like `QueuedConnection` cannot b

- **2022-07-03** (53 msgs, .roest, arcnor) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Modding / Data Files` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Save / Load` `Git / Version Control`
  after loading everything related to the construction of sprites from the DB (I've converted the DB calls to return classes from `dbstructs.h` that already existed, but were not used, sort of like a fa

- **2022-07-04** (18 msgs, ext3h, arcnor) `Database / Data Model` `Rendering / Graphics` `Pathfinding / AI` `UI / GUI Framework` `Performance / Optimization` `Platform / Portability`
  Hrm, even though I'm somewhat wary of the ranges library for different reasons. We had a chance to get proper autoparallelization into pretty much all STL containers and on most algorithms based on pr

- **2022-07-19** (122 msgs, ext3h, .roest, arcnor) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Save / Load` `World Generation` `Networking / Multiplayer` `Git / Version Control`
  Hrm, need to store a list of textures and offsets for each unique sprite id currently in the active set. As well as masks for all the source sprites, as we don't want to burn excessive bandwith on tra

- **2022-07-25** (269 msgs, ext3h, .roest, arcnor) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Save / Load` `Networking / Multiplayer` `Git / Version Control`
  ```cmake if(UNIX AND NOT (DEFINED ENV{CXXFLAGS} OR CMAKE_CXX_FLAGS))     set(CMAKE_CXX_FLAGS "-Wall -Wextra -fpermissive  -g3 -ggdb")     # Kill some overly verbose warnings     set(CMAKE_CXX_FLAGS "$

- **2022-07-26** (58 msgs, ext3h, arcnor) `Database / Data Model` `Pathfinding / AI` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Architecture / Refactoring` `Platform / Portability` `Save / Load` `World Generation` `Git / Version Control`
  Pondering what to use for the message queue... Leaning towards  `std::unique_ptr<std::function<void(void)>>` as the base container template, but still pondering how much flexibility I should account f

- **2022-08-16** (76 msgs, ext3h, arcnor) `Rendering / Graphics` `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Bug Fixes / Debugging` `Platform / Portability` `Git / Version Control`
  ``` # For a list of common variables see https://github.com/microsoft/vcpkg/blob/master/docs/maintainers/vcpkg_common_definitions.md  # Download source packages # (bgfx requires bx and bimg source for

- **2022-08-19** (11 msgs, ext3h, arcnor) `UI / GUI Framework` `Modding / Data Files` `Platform / Portability`
  hmm, nope, but I can probably push it in a bit, although it's only replacing a few of the libs, not touching BGFX like you did

- **2022-08-20** (2 msgs, ext3h, gsihaj5) `Game Mechanics` `Bug Fixes / Debugging` `Git / Version Control`
  For the most part by forking https://github.com/rschurade/Ingnomia on Github, and the opening a pull request for your contributions. As for what you can do? Partially depends on what skills you alread

- **2022-08-22** (2 msgs, ext3h, gsihaj5) `Game Mechanics` `World Generation` `Git / Version Control`
  The generated solution references all the sources from the git managed directory, there is no "second version". You can't change the solution or project files themselves though, that has to be done vi

- **2022-09-01** (35 msgs, ext3h, arcnor) `Database / Data Model` `Rendering / Graphics` `UI / GUI Framework` `Bug Fixes / Debugging` `Save / Load` `Git / Version Control`
  I'm puzzled regarding whatever the shader compiler did for wold_FS with the number of samplers.  > renderer_vk.cpp (4917): BGFX     sampler: uUndiscoveredTex (sampler1), r.index   4, r.count  1, r.tex

- **2026-02-24** (5 msgs, clawthorn, mugofjoe, ald_productions) `Pathfinding / AI` `Modding / Data Files` `Game Mechanics` `Bug Fixes / Debugging`
  Has anyone been looking into implementing Claude code or alt. AI for the sake of modding? Currently I’m putting together a piecemeal build of gnomoria with the aims of making a 2.0. With the help of C

## Database / Data Model

- **2018-08-25** (28 msgs, .roboute, .roest, socramazibi) `UI / GUI Framework` `Modding / Data Files` `Bug Fixes / Debugging` `Save / Load`
  honestly i'm not really sure that works, i did that like a year ago but in  Documents\My Games\Ingnomia\settings\config.json there's an entry language

- **2018-08-26** (11 msgs, currydays, shapkees, .roest) `UI / GUI Framework` `Modding / Data Files` `Architecture / Refactoring`
  it's a bit early to start with that anyway, too much stuff is still changing and many things are added, so it would be a nightmare to keep translation mods up to date

- **2018-12-24** (11 msgs, fantasticteaman, .roest) `Rendering / Graphics` `Modding / Data Files` `Bug Fixes / Debugging`
  Does anyone know if there's a way to view everything below a z-level? It would be easier to manage things if we could see like 10 z-levels lower than whatever the current level is. I think Gnomoria di

- **2019-01-19** (15 msgs, saint_of_grey, .roest, kondain) `Modding / Data Files` `Game Mechanics` `Platform / Portability`
  hmm theoretically you could, though at some points where I've been lazy/or simply didn't get to it yet it will look strange, like the Gnome List will still be called gnome list even if it's skeletons

- **2019-01-20** (42 msgs, .alch, yksleets, .roest) `Modding / Data Files` `Game Mechanics` `Git / Version Control`
  I'm thinking the game is in version 0.55.x and receiving more or less daily updates. making a mod on something that will most likely break tomorrow is a little optimistic

- **2019-10-25** (14 msgs, .roest, .mechworrier) `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Bug Fixes / Debugging`
  then of course you should remove everything from the json you don't want to mod and then use that json in the mod as in the example mods, should be pretty much self explanatory

- **2019-11-24** (5 msgs, gamerase, .roest) `Modding / Data Files`
  I am trying to add a new item that can be crafted in the Kitchen, so I am trying to update the record in Workshops. However, while adding a new row to the table works fine, I have had no luck with eit

- **2020-08-13** (11 msgs, .roest, tacyn.) `Bug Fixes / Debugging` `Architecture / Refactoring`
  Why is there "QString materialSID = *materialSIDs.begin();" at line 1000 in spritefactory.cpp instead of of just using materialSID as it is?

- **2020-10-25** (4 msgs, dinosaure.plz_nofriendrequests, .elfu) `UI / GUI Framework` `Game Mechanics`
  Easiness of hit is usually the leading parameter. The bigger the button, the easier it is to click. This is common to mobile apps too. Desktop buttons have the benefit of pinpoint mouse cursor so they

- **2020-11-21** (23 msgs, fris0uman, .roest, ext3h) `UI / GUI Framework` `Game Mechanics` `Bug Fixes / Debugging` `Save / Load` `Git / Version Control`
  So I tried to: make a crude workbench > order 10 plank > gnome get to work > kill that gnome > new gnome take his place Is there actually a menu somewhere to asign a gnome to a bench? because the auto

- **2020-12-03** (7 msgs, dinosaure.plz_nofriendrequests, ext3h) `UI / GUI Framework` `Bug Fixes / Debugging` `Architecture / Refactoring` `Git / Version Control`
  <@!406873466537508864> I've made a separate PR for a draft in which I tried to set resources as part of a .resx file. That looks kinda worky for MainPage.xaml, but I've not the time to finish this. Ad

- **2020-12-04** (16 msgs, dinosaure.plz_nofriendrequests, ext3h) `UI / GUI Framework` `Bug Fixes / Debugging` `Architecture / Refactoring` `Platform / Portability` `Networking / Multiplayer`
  Hmm, on Windows env, I'd use `GetLocaleInfo` from `winnls.h`. https://docs.microsoft.com/fr-fr/windows/win32/api/winnls/nf-winnls-getlocaleinfoa?redirectedfrom=MSDN Sorry, I'm really too far away from

- **2020-12-29** (5 msgs, njoyard, .roest, cravenwarrior) `UI / GUI Framework` `Git / Version Control`
  Hi there,  Lately I've been working on auto-generated docs for items, workshops, etc. from the db, initially for personal purposes (ie. learning the game), but I figured it might be helpful for others

- **2020-12-30** (14 msgs, harminoff, lekkimsm2500, ext3h) `Rendering / Graphics` `Game Mechanics`
  <@!520653029955862528> Sprite definitions are spread over two (or technically 3) tables. `BaseSprites::SourceRectangle` tells you *where* to sample *from*, while `Sprites::Offset` then tells you how t

- **2021-01-02** (5 msgs, ext3h, njoyard, .roest) `Rendering / Graphics`
  <@!520653029955862528>  Looking great with the material animations and the bases. Got to admit, I actually also used your page a couple of times as reference by now.

- **2021-01-07** (37 msgs, ext3h, njoyard, .roest) `Rendering / Graphics` `Bug Fixes / Debugging` `Architecture / Refactoring`
  <@!520653029955862528> https://developer.mozilla.org/en-US/docs/Web/CSS/image-rendering with setting `pixelated` or `crisp-edges` should better correspond to the ingame rendering look. The default smo

- **2021-01-12** (3 msgs, njoyard, .roest) `Architecture / Refactoring` `Git / Version Control`
  nice, do you ever feel like working on ingame stuff? we still have a lot of unused sprites that need definitions 🙂

- **2021-01-25** (9 msgs, ext3h, rsmith32) `UI / GUI Framework` `Bug Fixes / Debugging`
  I think I get what your saying. Just to clarify  AggregatorInventory::updateWatchedItem  is causing an element in GameState::watchedItemList to be modified or deleted in a way that when it returns to 

- **2021-01-30** (27 msgs, condac, ext3h, .roest) `UI / GUI Framework` `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring`
  As my simple approach is now when a gnome starts a task it triggers the sound to be played. And then itt calls my playEffect function that checks if the sound should be played based on range to the ob

- **2021-02-04** (2 msgs, condac, ext3h) `Modding / Data Files` `Game Mechanics` `World Generation`
  It would be nice if there was a custom farming zone so things like beehives and custom mods can use and create things that do not require dirt soil. As it is now the bees are custom code but it can ju

- **2022-07-17** (7 msgs, .roest, arcnor) `UI / GUI Framework` `Modding / Data Files` `Platform / Portability`
  some things like those "New Preset" and similar buttons are cutoff, although I can't remember if it was like that on Windows as well, plus there is a strange blue "flash" when scrollbars are rendered 

- **2022-07-27** (11 msgs, ext3h, arcnor) `Performance / Optimization` `Bug Fixes / Debugging` `Platform / Portability` `Git / Version Control`
  also, for the whole 3rd party library issue, I've found we can probably use `vcpkg`, all libs I've added to the project are available on their public repo, and there is a "native" way to cache them in

- **2022-07-28** (5 msgs, arcnor)
  I've converted almost every "table" to a map, although I've had to remove a few duplicates in some cases, and some other places where I think it should be a map but duplicates were not clear I just le

- **2022-07-29** (9 msgs, hughheggen, arcnor) `UI / GUI Framework` `Architecture / Refactoring` `Platform / Portability`
  Well, unfortunately it means nothing will change for players much (except that one of my changes made the game work on Mac, and potentially other platforms like consoles, although I'm not sure if that

- **2022-09-02** (11 msgs, ext3h, arcnor) `Rendering / Graphics` `UI / GUI Framework` `Performance / Optimization` `Architecture / Refactoring` `World Generation`
  Consider the following in the shared interface between the front- and backend. ```c++ struct GuiPastureFoodItem {     QString itemSID;     QString materialSID;     QString name;     bool checked = fal

## Game Mechanics

- **2021-01-18** (1 msgs, rsmith32) `Git / Version Control`
  Hi! I just created a pull request for some minor Farm/FarmField optimizations. First time doing a pull request on git I think I did it correctly.

- **2021-01-24** (8 msgs, cptvolkow, rsmith32) `Performance / Optimization` `Bug Fixes / Debugging` `Save / Load`
  I was doing some testing replacing std::bind with lambdas when functions were being added to the  m_behaviors in animals. With some fairly crude performance measurements it seems like there was a perf

- **2022-08-04** (2 msgs, ext3h, arcnor)
  I'm still waiting for mine to begin...

- **2024-12-21** (1 msgs, sewwes)
  \o I'm working on a similar style game that I'm making for N64 hardware, quick question to see if I understand the GNU license. Mostly looking at the code base to help me structure mine in a similar m

## General Discussion

- **2018-10-06** (1 msgs, seronis1129)
  <@103489218805784576>  probably sometime after it becomes a priority.   There is no approx time on things that arent a priority in most projects unless you like the default  "Soon(tm)" answer which is

- **2018-10-22** (6 msgs, shapkees, hughheggen, prassel)
  https://developer.valvesoftware.com/wiki/Valve_Time

- **2018-12-21** (1 msgs, hughheggen)
  it's not popular enough for that I think

- **2018-12-23** (2 msgs, .roboute, optimus.0001)
  there's still game-breaking releases going on, when that's sorted I'm keen to try some tinkering

- **2018-12-26** (1 msgs, hughheggen)
  You mean you want to make undiscovered blocks transparent so you can see underneath?

- **2019-09-02** (2 msgs, shapkees)
  <@376881239296180236>  The overwhelming number of Russian-speaking players do not know the English language at the proper level and are ready to play WITH TRANSLATION OF ANY QUALITY, if only in their 

- **2019-09-04** (4 msgs, .alexgee, .alch, desolator_x)
  if someone wants to do a translation of a game that's still being developed that's on them. I'm pretty sure the translation is easier when the game is finished

- **2019-10-19** (1 msgs, .mechworrier)
  some small naked men

- **2019-10-22** (3 msgs, .roest, boris8681)
  no idea, why would it be gone? that stuff is on your machine

- **2019-11-25** (2 msgs, reaperofbunnies)
  I've been working in the foods and other items

- **2019-11-26** (1 msgs, fourofire)
  Does anyone know a steam modder called Fenrir?

- **2020-05-30** (1 msgs, dommain.)
  I am waiting for seasons and getting to see leaves on the grounds during fall, snow in the winter, and butterflies during spring.

- **2020-08-18** (1 msgs, shadmir)
  Oh it's Tacyn

- **2020-10-10** (5 msgs, dinosaure.plz_nofriendrequests, optimus.0001, xeridanus)
  "Embark" might be translated as "Embarquer", but that's a very boat/ship/vehicule use. And that may be seen as a very poor translation if the Gnomes comes without vehicules at all. "Embark on a journe

- **2020-10-11** (2 msgs, drucifer8036, xeridanus)
  Embark also has strong boat/ship/vehicle association in English but can be used for just a trip/journey. The Gnomes don't have a vehicle as far as I'm aware.

- **2020-10-19** (1 msgs, dinosaure.plz_nofriendrequests)
  I totally understand that, but the layout "LB4C"-based square width was a problem, since "designation" (EN lengthen text) translates to "zones" (FR shortest text).... Thought the "square-like" buttons

- **2020-12-08** (1 msgs, dinosaure.plz_nofriendrequests)
  I'm leaving Ingnomia for the moment, see you one day or another.

- **2021-01-11** (1 msgs, njoyard)
  (edited) having fun refactoring docs sprites to be able to do more stuff

- **2021-01-15** (4 msgs, ext3h, njoyard)
  Likely because the internal representation of the canvas is in linear color space, so you get unavoidable rounding errors both even loading and saving.

- **2021-01-22** (2 msgs, njoyard, .roest)
  starting to get the hang of it

- **2022-03-16** (2 msgs, desolator_x)
  this is the kind of stuff you do a few weeks before "release"

- **2022-07-15** (2 msgs, arcnor)
  Been replacing more code, have added a few more libs in the end 😁

- **2022-07-21** (1 msgs, arcnor)
  I've messed up the channels somehow but now images are finally showing 🙂

- **2022-08-05** (3 msgs, xeridanus, arcnor)
  I'm also a freelancer, so I can decide to slack off without repercussions! (Except a worrying lack of income, of course 😛)

- **2022-08-21** (1 msgs, gsihaj5)
  i'm a programmer, but never touch this kind of large open source project. thanks for the information, i'll read more about the instruction and try to experiment on my own.

- **2022-08-23** (1 msgs, gsihaj5)
  i see, thankyou

- **2024-12-23** (2 msgs, Deleted User)
  Also, this sort of game requires a lot of processing power and RAM for older hardware. There is a lot of pathfinding running constantly and a lot of items to keep track of.

## Git / Version Control

- **2021-01-26** (2 msgs, rsmith32)
  Also, curious what your thoughts are on making the object class extend from QObject. We would be able to implicitly do what were currently doing with the CreatureType enum. We would also be able to wr

- **2021-05-29** (2 msgs, njoyard, nardian13)
  I don't know which download you used but it really looks like it's too old. In particular the grass tilesheet was changed (to a 32x36px grid) after the last download available from github. You may wan

- **2022-08-18** (9 msgs, ext3h, arcnor)
  Cool, do you have a branch with all the vcpkg stuff? Otherwise I can open one on top of what I've done

## Modding / Data Files

- **2018-05-18** (3 msgs, tacyn., dragonwrath420)
  <@358668557074300942> Just curious but do you already have some larger mods in the pipeline/works/progress for ingnomia to take advantage of the new stuff/engine (such as multicore support, better fil

- **2018-12-12** (1 msgs, .roest)
  example mod to change styles, just unzip in Documents\My Games\Ingnomia\mods, rename and start modifying

- **2018-12-16** (1 msgs, socramazibi) `Game Mechanics`
  I have put the mod in the folder that you comment and it does not work, instead I put it in the Steam workshop mod folder and it works

- **2018-12-20** (1 msgs, koogen)
  I've been following this game for a while now, honestly surprised the modding scene hasn't exploded yet

- **2019-09-27** (4 msgs, shapkees, .roest, desolator_x)
  well, the option to mod is already in the game, so if someone wants to go ahead and translate the game in the language of their choice that's up to them

- **2019-10-10** (3 msgs, .roest, Deleted User)
  i absolutly don't know how to mod, but i can do the translate to French language if someone can create the mod ^^

- **2019-10-21** (1 msgs, .roest)
  modding is back on indev

- **2019-11-01** (1 msgs, .mechworrier)
  is the 32x32px grid size baked into the code, or is that something that could be increased with available config changes?

- **2021-01-06** (17 msgs, dinosaure.plz_nofriendrequests, njoyard, .roest) `World Generation` `Git / Version Control`
  Because of the cold in the winter months they do this initial stage in a warmer more suitable setting for growth so they have a better chance of growing in to full trees in a generally colder climate

- **2022-03-08** (10 msgs, .roest, cptvolkow, phoenixfice) `Game Mechanics`
  And otherwise, a tip for the steam page, for the description of the game, this one doesn't give any info on the game except "it's the sequel to Gnomoria", not everyone knows Gnomoria so it would be ni

## Networking / Multiplayer

- **2018-10-20** (2 msgs, guri9486, prassel)
  Soon: https://developer.valvesoftware.com/wiki/Valve_Time

- **2019-08-13** (7 msgs, .alexgee, Deleted User)
  Well, I see no problem (programming issues aside) in giving the option for the community of players to translate it to additional languages.

## Pathfinding / AI

- **2020-11-23** (9 msgs, dinosaure.plz_nofriendrequests, .roest, ext3h) `Game Mechanics` `Bug Fixes / Debugging`
  Fair word of warning, the definition of the `Star`-suffixed nodes is currently inverse to the definitions of these nodes in Groot. Maybe we should fix that at some point, so we can just point to their

- **2020-11-24** (24 msgs, dinosaure.plz_nofriendrequests, ext3h, .roest) `Modding / Data Files` `Game Mechanics` `Performance / Optimization` `Git / Version Control`
  Next up, not "lag spikes", but "constant lag", querying groves and farms for jobs, which is currently `num_gnomes * num_tiles` time complexity, rather than `num_gnomes + num_tiles` as it should be.

- **2021-01-27** (8 msgs, ext3h, rsmith32) `UI / GUI Framework` `Game Mechanics` `Performance / Optimization` `Bug Fixes / Debugging` `Architecture / Refactoring`
  Qts reflection system uses strings for runtime lookups? yikes.  Unreal provides a pointer to a class type object. After I typed that out I also saw that items extended from object as well which your t

- **2026-02-25** (1 msgs, clawthorn) `World Generation` `Networking / Multiplayer`
  I don't think you'll find anyone in here. The server is pretty inactive, and from what I've seen the gamedev (and gamer) community as a whole leans heavily against AI. As does pretty much every creati

## Platform / Portability

- **2020-10-03** (1 msgs, deanec64)
  anybody made a Linux release as of yet?

## Rendering / Graphics

- **2017-11-11** (8 msgs, .roest, bul1) `Modding / Data Files` `Game Mechanics`
  https://www.reddit.com/r/ingnomia/wiki/modding

- **2019-11-02** (15 msgs, .alch, .roest, .mechworrier)
  but if you try to make a model of a guy, then add a breathing animation or something, it would look pretty wonky if the eyes or hands keep chopping off because of the movement

- **2020-11-22** (35 msgs, fris0uman, ext3h, .roest) `Pathfinding / AI` `Game Mechanics` `Bug Fixes / Debugging` `Save / Load` `Git / Version Control`
  Basically will this actually empty all equipment and drop it? ```cpp void Creature::dropEquipment() {     Inventory& inv = Global::inv();     for ( const unsigned int it : m_equipment.wornItems() )   

- **2021-01-01** (5 msgs, ext3h, cravenwarrior)
  i was just researching Vulkan some since itd make a Metal-compatible version less ridiculous, figured out that Intel graphics dont support it below 6th gen

- **2021-01-14** (5 msgs, ext3h, njoyard, .roest)
  -_- just did a bunch of moving around sprites, and discovered that the thing I did with html canvas does very subtle color changes when saving images... hardly visible, but still. have to find out why

- **2022-05-18** (3 msgs, cptvolkow, montanoc70) `Git / Version Control`
  how do i donwload the openGL as it says in the github?

- **2022-07-23** (6 msgs, .roest, arcnor) `Save / Load`
  the dry run creates the hash key to avoid baking a random sprite again, also to be able to reproduce the same random sprites after load

- **2022-07-24** (10 msgs, .roest, arcnor) `Game Mechanics` `Bug Fixes / Debugging` `World Generation`
  1 sheet of packed atlas texture, using 2048x2048 for easier/faster debugging, but as mentioned, I think it should be 4kx4k as a minimum, but I've changed all sprite calculation to be dynamic depending

## UI / GUI Framework

- **2018-07-19** (1 msgs, mr_mauve9582) `Architecture / Refactoring`
  Are there any plans to hook functions and create an interface so modders can write actual mods?

- **2020-10-09** (49 msgs, dinosaure.plz_nofriendrequests, rivenwyrm, ext3h) `Modding / Data Files` `Game Mechanics` `Architecture / Refactoring` `Git / Version Control`
  Could you explain a little lore about the "Embark" verb used to start a new game? Are the Gnomes supposed to come from a ship (floating boat? spatial shuttle?), or any other form of vehicle? Else, doe

- **2020-10-13** (2 msgs, dinosaure.plz_nofriendrequests)
  <@!284497763448651797> "s'aventurer" has something risky in its meaning. Likely "Face danger", or "Brave adventure". That genuinely means "go on an adventure", but it includes danger-triggering sign. 

- **2020-10-14** (13 msgs, dinosaure.plz_nofriendrequests, xeridanus, .roest) `Game Mechanics` `Git / Version Control`
  i think the button should say `Si vous appuyez sur ce bouton, votre joyeuse bande de gnomes s'embarquera dans une longue aventure où ils formeront des amitiés, combattront des monstres et amasseront d

- **2020-10-21** (36 msgs, dinosaure.plz_nofriendrequests, ext3h, .roest) `Architecture / Refactoring`
  "Width" is the size you want for your control; min is the minimal size you want for your control, that said if the width is "Auto" (or not set), this is the minimal value the "Width" will have; max is

- **2020-10-23** (3 msgs, dinosaure.plz_nofriendrequests) `Architecture / Refactoring` `Git / Version Control`
  Interesting article. I'm quoting this particular sentence as I'm reading further into it: > In general, the more flexibly you can design your layout, the better. Allow text to reflow and avoid small f

- **2020-10-24** (8 msgs, troa_barton, .roest, dinosaure.plz_nofriendrequests) `Architecture / Refactoring`
  <@!494607567415410698> there's a large amount of people who have a problem with that kind of UI (towns without text) that's also how the old gnomoria gui was and that led to most of them using the rig

- **2020-11-07** (2 msgs, dinosaure.plz_nofriendrequests)
  Hello. For now, I didn't manage to make the translation work with current c++ culture as I don't have spare time anymore. Somehow, I may try to do it one day, if noone made it already. I'm not forgett

- **2020-12-13** (4 msgs, ext3h, cptvolkow) `Performance / Optimization` `Architecture / Refactoring` `Save / Load` `Git / Version Control`
  Also clean isolation of the simulation and the UI thread, something we are going to need at some point next year for *--top secret plan--*.  In the mean time, this will hopefully enable us to better t

- **2022-06-29** (1 msgs, loyalviggo) `Git / Version Control`
  Hello all. I'm a Junior Software Developer, but unfortunately not in C++ or XAML (yet). I'd be interested in helping other areas so if anyone can point me to a resource (outside of the repo perhaps) w

- **2022-07-18** (3 msgs, ext3h, arcnor)
  Hmm, I downloaded whatever was on their website

