# OpenLoco version 23.06 (???)

## Hardware backed window scaling (#1965)

OpenLoco has had integer window for a few years now. A frequent request was to allow scaling at half or other fractional scales, however. This was already possible if players cleverly manipulated their config files, but the resulting scaling didn't look very nice -- which is why the game UI did not expose it. Making it look nice, however, meant reimplementing the SDL canvas backend to use GPU hardware display for our software renderer. @ZehMatt was up for the task, and made it work very nicely indeed. Check it out!

## Regressions from new clear command (#1967, #1980)

Our last release saw us implementing the 'swiss army knife' of construction functions. Our testing didn't reveal any problems, but shortly after the release, it became clear that this was too good to be true. In fact, there were two. Players could no longer change land textures -- thankfully a minor problem -- but certain 'clear' functions would crash the game entirely. Some debugging, both problems have been eliminated. Hopefully, v23.06 will prove to be more stable!

## Clear tile function (#1959)

Having implemented the 'swiss army knife' of construction functions, @duncanspumpkin implemented the bulldozer tool's clear tile function. Many game commands use instructions like this, so it is great to have a good, working example for implementing the rest.

## Make the news sound optional (#1963)

Locomotion's sound effects have a lot of charm to it, including the sounds associated with the monthly news messages that come in on occassion. Admittedly, though, after hours of "testing" the game the 'oooh's and 'aaah's can get on ones nerves. We've finally introduced an option to disable these sound effects entirely. The option can be found in the news options tab in the messages window.

## Vehicle route order command reimplementation (#1961, #1962, #1977, #1981)

The game commands dealing with the vehicle route order table have all been reimplemented. This means inserting, deleting, and moving up/down of vehicle orders now happens entirely in C++. Reimplementing these game commands is all working towards a unified interface with multiplayer games in mind.

## Vehicle speed control and refit command reimplementation (#1954, #1958)

Similarly, we have reimplemented the game commands dealing with vehicle speed control (manual driving) and refitting of ships and aircraft (cargo). Nice!

## Stream class refactors (#1979, #1982)

Many parts of OpenLoco use custom stream classes for interacting with binary game data. The initial implementations, a few years old at this point, sufficed for what they were doing, but were not very optimised. @ZehMatt set off to split them into separate unit files, streamlined their interfaces, added tests, and has started work on optimising them. The initial benchmarks look very promising. To be continued!

# OpenLoco version 23.05 (2023-05-27)

## Construct With Clear (#1899)
Two months ago, @duncanspumpkin started working on this with the comment "This is going to take a little while" -- and indeed it did! The construct with clear function is a core part of many Game Commands. This function is a bit of a swiss army knife. It can tell you if the location you are trying to construct is underwater/underground/overground/clashes with ground, or if there are tile elements in the way. It can also be further customised to remove elements as it's checking for clearance. Getting this to interop with the existing game was a challange, but now that it's done, it opens up implementing many of the placement Game Commands, e.g. CreateBuilding.

## Fix C++20 Issues (#1932)
Currently the codebase is built using the C++17 standard. We would like to move to C++20 in the short-term, as there are some great new features in the newer standard. This PR addressed many of the compile issues. There is now only one area left to address, but unfortunately it's the biggest problem: moving to `char8_t` and `std::u8string`. We will need to make a number of UTF-8 related changes to really be ready for C++20, but it is getting nearer. Expect a number of UTF-8 related changes over the next month. #1937, for example, brings UTF-8 to the Windows command-line.

## Refactoring Game Command Args (#1914, #1915, #1917, #1918, #1919)
Continuing from last month, @reinaldorauch split up his big argument refactor into a number of bite size chunks. We are now starting to have a consistent interface for Game Commands. There is still a good many left to refactor, though. This is a good area for newcomers, so contributions are always welcome.

## Construction window bug (#1940)
We'd like to bring special attention to #1940 by @LeftofZen. While it started out as a refactor PR, centralising some memory handling, it accidentally fixed a very vexxing bug that made using the construction window impossible under certain situations. It was caused by a type issue in a `loco_global`, which was refactored out by this PR. This goes to show how important strict types are while interopting with vanilla code! We're glad to fixed at last, as it had been bugging us for too long.

## Misc. Refactors (#1926, #1921, #1943, #1950, #1951)
A handful of minor refactors were contributed by @memellis and other team members, enforcing our newer code styles: enum class flags, and using our new logging library.

# OpenLoco version 23.04.1 (2023-04-27)

Whilst implementing one of the vehicle game commands for v23.04 a refactor went slightly wrong causing the local/express setting of a train to constantly be reset. Woops! Eventually we hope to have a proper testing system that would capture this sort of mistake but its hard to add such a testing system whilst we depend on the base game.

## Pack all custom objects (#1925)

Many custom objects for locomotion were made by modifying an existing vanilla object. Unfortunately most object makers did not modify the base game field to indicate that it was a custom object. This meant that the object would not be packed into the save even when the export objects option is used. We now check if an object is from the base game by comparing it to the complete list of base game objects ensuring that they are all packed.

## Introduce object load error window (#1933)

It can be incredibly frustrating when loading a save only to find out you don't have all the objects installed to load it. In the base game an error message was generated that told you of only one missing object. We now display a window for all the missing objects.

# OpenLoco version 23.04 (2023-04-23)

## Refactor arg structs for game commands (#1893, #1916)

As part of our ongoing efforts to reimplement all game commands in C++, we are refactoring the game command's argument structs to be higher-level. Currently, most are still passing registers into vanilla functions. We have devised a wrapper structure to make our C++ code more readable in the mean time, while at the same time preparing for command serialization for future multiplayer.
This month, @duncanspumpkin and @reinaldorauch tackled a bunch of these refactors, even if not all of them have been merged yet.

## Implement two more vehicle game commands (#1923, #1924)

Related to the previous section, @AaronVanGeffen implemented two more vehicle game commands: the vehicle 'reverse' command, and the vehicle 'pass signal' command.

## Improved diagnostics and tests (#1894, #1904)

@ZehMatt reworked our console logging interfaces into a new Diagnostics module. This allows log messages to easily be saved to file for reference, and for easier extension and reuse in the future. In addition, more tests for our custom string functions were added. Hopefully, this will prevent regressions from popping up when we switch to Unicode for internal strings in the future.

## Switching from Docker Hub to GitHub for CI images (#1896)

To the unpleasant surprise of many open source projects, Docker Hub announced that they were ending their free tier for open source teams. This prompted us to look for an alternative, which we found close to our project's home on GitHub. @janisosaur steered this direction to a good end.

## More vehicle flag refactors (#1898)

For a healthy code base, and in anticipation of a future new save format, we have been working on using strict types as much as a we can. Owing to @ZehMatt's recent innovations for flag typing, we have been slowly replacing separate constants with *enum classes* over the past few months. This month, @reinaldorauch refactored more of the vehicle flags.

## Allow hooking non-aligned function addresses (#1907)

As a result of the implementation of last month's town growth functions, we discovered some processors had trouble with us hooking functions that would misalign address-wise on particular processors. Notably, our Windows builds were no longer working through Wine on macOS. Thanks to @janisosaur's detective work and swift fix, this is working once again.

## GameState integration (#1911, #1912)

OpenLoco's code base has been relatively good in its relatively small use of globals. Over the past months, we have been reducing them further back into a *game state*. This month, a few more globals were integrated into the game state struct once again.

## Dealing with compiler warnings (#1913, #1920)

For a healthy code base, we aim for our code to compile without warnings on the GCC, clang, and MSVC compilers. This month, a pesky warning in the paint routines was vanquished. We can compile without warnings, once again!


# OpenLoco version 23.03 (2023-03-16)

## Implement remaining object loading functions [#1851, #1848]
At the start of this month, we finished the remaining object loading functions. The last two were the hardest ones: Vehicle and Tree. Tree might be a bit of a surprise to have such a complex loader, but it's all to do with the various seasons that trees can have, as well as their shadows. Due to the complexity of these two, we made a few mistakes, but we think we have managed to iron out all the bugs prior to the release.

## Implement Town Grow Functions [#1740, #1742]
Many people have mentioned that they don't like how towns grow in the game. Before we can overhaul how they work, we need to first implement the existing behaviour. Unfortunately, the main town growth function is large spaghetti of code, so to make it as simple to implement as possible, we are going to implement all of its calling functions before tackling the main function. This helps us understand the main meat of the function.

## Project layout [#1850, #1855, #1859, #1867, #1868, #1873, #1879]
@ZehMatt has been continuing his work with laying out our codebase in a more logical manner. Along the way, FmtLib has been introduced to OpenLoco. FmtLib is the basis of the C++20 std::format, and it is a very nice library for formatting text. It should lead to much neater code for error messaging and logging. This, in turn, will mean we will likely write more error messages and logs, as it's much easier to write.

## Implement Final Window Function [#1881]
Not only did we finish off all of the object load functions this month, we also finished off the last window function, the Station Tool's update event. The tool update functions are used to show the ghost previews of the tool you have active. Stations are quite complex, as you have rail, road, airports and docks. This leads to complex code, hence why this final function had been ignored until now. Now that all the functions have been implemented, we can start removing the legacy interop callers, and rearrange parameters into more logical structures. There are still a good many functions that use windows indirectly, so we do still have some constraints, though.

## Implement Parts of Clear Land [#1872]
@AaronVanGeffen has been chipping away at the clear land game command. It's a surprisingly tricky one, so it's been split into a couple of chunks. Hopefully, next month we will have it over the line.

## Implement Drawing Engine Functions [#1836, #1861, #1862]
@niceeffort had a shot at implementing the fillRect function for the drawing engine. This function, surprisingly enough, draws solid rectangles. It has a number of additional modes for different patterns though, which makes the code a little complex. Luckily, it's all very similar to OpenRCT2. In addition, we finished off the missing draw string centred function.

## Implement Remove Tree [#1866]
Another @niceeffort function implemented. This time, it's the low level tree removal function. One interesting detail about removing a tree is that the destruction sound has its pitch adjusted by a random amount. This pitch adjustment means that removing 100s of trees at once doesn't produce a high volume sound. Neat!

## Enumeration flag conversion [#1778, #1819, #1891]
Continuing from last month, @reinaldorauch, @niceeffort, and @leslieyip02 converted the last few remaining flags to the stricter enum classes. Along the way, one or two mistakes in existing code were noticed and fixed, showing the refactor isn't just for aesthetics. Great work!

# OpenLoco version 23.02 (2023-02-19)

## Implement object loading functions [#1822, #1824, #1829, #1834, #1842, #1846]
This month, our journey towards implementing the object load functions continues. As of this release, we're nearly at the finish line! As we noted in last month's development log, these loading functions handle data such as sprites, animations, and localisable strings. Implementing these functions will eventually make it possible to introduce our own object format, increasing limits further down the line. Exciting stuff to look forward to!

## Enumeration flag conversions [37 separate PRs]
This month, @niceeffort converted most of the flags in OpenLoco to use strict enumeration class types. Building on the work introduced in the past month, these new types make it easier to spot mistakes -- both for ourselves, and the C++ compiler. Ultimately, this should lead to less bugs, which is nice for everyone :-)

## Object selection window improvements [#1833, #1837]
Players with many objects (mods) in their Locomotion folders will often have to spend much time looking for particular objects when designing custom scenarios. No longer! You can now find objects by display name or filename, simply by typing in the new filter box. With the object selection window having been made available in-game since December 2022, this should make scenario play even more fun.

## Split off more code into modules [#1771, #1785, #1800, #1844]
A longtime contributor and friend from the OpenRCT2 project, @ZehMatt, recently joined forces with us to help out moduralise the OpenLoco project further. This helps us compartmentalise and test the code base better, ultimately leading to better, more reusable code.

## Right-click panning is smooth again [#1475]
A bug that has been plaguing OpenLoco for a while is the map panning being slow when the 'uncap FPS' option were disabled. It turns out to have been introduced by that very same option, making this a regression. Thanks to @ZehMatt, the bug has been vanquished, making the game buttery smooth while panning, once again.

## Renderer code improvements [#1766, #1796, #1799, #1835, #1838]
Another of @ZehMatt's projects has been the UI rendering. While OpenLoco code already takes care to split core game logic from UI logic, we have had difficulty to split UI code from the viewport (map) buffers. @ZehMatt's recent efforts will ultimately make it possible to scale the UI separately from the rest of the game. Very exciting indeed!

# OpenLoco version 23.01 (2023-01-25)

## Implement Object Loading Functions [#1739, #1746, #1747, #1749, #1752, #1753, #1758, #1759]
The main focus of this month has been implementing the object loading functions. When the game loads an object from file it has a header which specifies many of the properties of the object but there are a number of fields within the header that are left empty. It is the object loading function that fills in those empty spaces. In general it is for dynamically sized data such as strings, images and animation maps. Locomotion has been designed cleverly so that these dynamic elements are in one memory allocation. A singular memory allocation improves the loading time and may also marginally improve the game speed. Almost all of the object loading functions have now been implemented but a few of the more tricky ones remain for next month.

## Config Enum Flags [#1760]
Special thanks to @leslieyip02 for this pull request (and his future one[s] that will be into next months release). This was start of rolling out the Enum Flags that were added last month.

## Improve the OpenLoco First-Run Experience [#1764]
In order for OpenLoco to run, the original Locomotion game asset files are needed. In many cases, the game is able to automatically detect its location. However, if that wasn't the case, a prompt would be displayed asking to locate the Locomotion folder manually. The phrasing for this prompt was quite spartan, however, which led to some confusion. We have rephrased the prompt messages, so the first-run experience should be markedly improved as of v23.01.

# OpenLoco version 22.12 (2022-12-22)

## Introduce Enum Flags [#1741]
One of the limitations of C++ is that there is no nice native way to handle flags for a type. There are many alternative ways of doing it all with positives and negatives. For the history of the project we had been using a namespace of constexpr values. This was easy to use but it wasn't type safe. @ZehMatt suggested using Macro to add helper functions to a traditional enum class. This improves the type safety of the flags and reduces the chance of accidental bugs. Next month we will roll out this change to all of the flags of the game to ensure consistency.

## Implement Object Image Table Loading [#1733]
Almost all .DAT files contain an image table for the sprites used in game. Depending on the type of the object this table has a certain size. Implementing the loading function has helped identify a minor new feature compared to RCT2. Images it appears can have a flag to indicate that they are a duplicate of the previous image. We aren't entirely sure why this feature is required but its interesting.

## Implement S5 functions [#1726]
These functions are used to quickly read the main headers of save files and scenario files. The details are then used for display in the Save Selection and Scenario Selection windows. This represented some of the last of the S5 loading functionality and paves the way for introducing our own save format!

## Support Building Flatpak Images (Linux) [#1711]
Special thanks to @garethppls for this pull request. These changes to the CMake allow for OpenLoco to be built as a flatpak image for ease of use on the Linux platform. The next stage of this will be to add a CI job to create flatpak images automatically.

## Split off Utility and Core Libraries [#1713]
After last months CMake refactoring it was finally time to start splitting up the repository into a number of libraries and adding unit tests. After extracting Utility and adding tests it identified a number of edge cases we hadn't accounted for in the libraries so it hopefully will reduce bugs. There is still plenty to move into these two libraries and plenty of tests to add but its a start.

# OpenLoco version 22.11 (2022-11-20)

## Implement Industry Functions [#1643, #1669, #1694, #1700, #1707, #1716]
A number of pull requests all focused on industry related functions. These represent almost all of the remaining industry functions. All that remains for industry is to implement CreateIndustry GameCommand and IndustryObject load. At this point almost all of the industry related structures have been mapped and are mostly understood. There is not much that is not known about industries.

## Misc Refactors [#1706, #1705, #1701, #1715]
No code base is without violations to coding style or minor design issues. These refactors help ensure the codebase is consistent, sensible and compiles quickly. All of which improve the speed at which new features can be implemented and code can be understood. Special thanks to @tnixeu who is not part of the regular team for his refactor.

## Rename free/get ScenarioText and implement them [#1697, #1720]
The name of these functions come from our original work on OpenRCT2. The name was badly given in OpenRCT2 but it stuck with the codebase for a long time. We used the same name in OpenLoco but it is now finally renamed to load/free TemporaryObject. Which is much clearer as to its purpose.

## Project Layout Change CMake [#1686]
The whole project has been reorganised and the majority of the CMake scripts have been rewritten to allow for modularising parts. This will allow us to write unit tests for sections of code. In addition on Windows VCPKG will automatically download and then download all of the dependencies. This allows for a much simplified developer experience. Unfortunately this has meant that we have had to retire the old MSVS .sln and .vcxproj files. All building of the project now happens through CMake.

## PaletteMap Cleanup [#1675]
Internally the game draws everything with a 256 colour palette. To handle transparencies and shadows the game has a number of maps that are used to apply these transparency effects. This PR cleaned up how the palettes were used and identified a major source of use after free which could potentially cause issues. Luckily there were no issues but its all much clearer now.

## Add Loco String Functions [#1642]
Internally locomotion uses its own form of strings. These strings can have inline parameters and arguments or arguments passed as a separate variable. To achieve this the game assumes that all text is a C-string, mostly ASCII (7bit) and uses some of the spare bits for argument indication (sentinels). This causes some havoc with trying to support non-Latin languages as we ideally need to move to UTF-8. The arguments vary from 1-4 bytes of information and crucially they might have 0 as one of those bytes. This potential 0 means that all C-string functions strcat, strcpy, strlen may work incorrectly and not understand the string. Therefore we now have locoStrlen, locoStrcpy to replace them. Eventually we will require to rework all string handling so that arguments are safely encoded in (hopefully) UTF-8 but that is a long way off.

# OpenLoco version 22.10 (2022-10-09)

## Change Company Face Game Command [#1656]
Changing the company face is much more complex than you might think. Each company face is a separate Object and changing the face requires unloading and loading objects. This is something that CS generally avoids and limits to the scenario editor. It gives key insight as to how to enable the object selection window in sandbox play and how best to network the object selection window in the future.

## Change Company Name Game Commands [#1640]
These game commands are pretty self explanatory. The implementation allowed for fixing some known buffer overflows in the original code. These were the last change name game commands to implement.

## Industry Manager Functions [#1605, #1621]
Implement various Industry Manager functions. These functions were used at the end of each month to build new industries if the game decided that there was not enough. It has helped to expose a large amount of the flags that an Industry Object can have and the information will be of use to any modders or min/maxers. IndustryManager::findRandomNewIndustryLocation is the key function for this.

## Refactors
A number of code style refactors and cleanups to help unify the codebase. There are luckily not many code style issues in the codebase anymore.

## Build
CMake is a build system that is the defacto standard in C++. We are slowly moving OpenLoco to being a purely CMake project and removing the MS .sln file. This will help unify the build experience across Windows and Linux. As an added bonus it will mean Linux devs do not need to understand the MS .vcxproj file format to be able to add new files or move files around. There is still further work to do though before removal of the .sln file.

## Misc
Added a development log (this). Most items in the project will be covered in the changelog but for items that do not normally warrant a changelog entry they will be captured under this log.

