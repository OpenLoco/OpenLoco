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

