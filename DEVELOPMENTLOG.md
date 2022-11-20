# OpenLoco version 22.10+ (???)

## Implement Industry Functions [#1643, #1669, #1694, #1700, #1707, #1716]
A number of pull requests all focused on industry related functions. These represent almost all of the remaining industry functions. All that remains for industry is to implement CreateIndustry GameCommand and IndustryObject load. At this point almost all of the industry related structures have been mapped and are mostly understood. There is not much that is not known about industries.

## Misc Refactors [#1706, #1705, #1701, #1715]
No code base is without violations to coding style or minor design issues. These refactors help ensure the codebase is consistent, sensible and compiles quickly. All of which improve the speed at which new features can be implemented and code can be understood. Special thanks to @tnixeu who is not part of the regular team for his refactor.

## Rename free/get ScenarioText and implement them [#1697, #1720]
The name of these functions come from our original work on OpenRCT2. The name was badly given in OpenRCT2 but it stuck with the codebase for a long time. We used the same name in OpenLoco but it is now finally renamed to load/free TemporaryObject. Which is much clearer as to its purpose.

## Project Layout Change CMake [#1686]
The whole project has been reorganised and the majority of the CMake scripts have been rewritten to allow for modularising parts. This will allow us to write unit tests for sections of code. In addition on Windows VCPKG will automatically download and then download all of the dependencies. This allows for a much simplified developer experience. Unfortunately this has meant that we have had to retire the old MSVS .sln and .vcxproj files. All building of the project now happens through CMake.

## PaletteMap Cleanup [#1675]
Internally the game draws everything with a 256 colour palette. To handle transparancies and shadows the game has a number of maps that are used to apply these transparancy effects. This PR cleaned up how the palettes were used and identified a major source of use after free which could potentially cause issues. Luckily there were no issues but its all much clearer now.

## Add Loco String Functions [#1642]
Internally locomotion uses its own form of strings. These strings can have inline parameters and arguments or arguments passed as a seperate variable. To acheive this the game assumes that all text is a C-string, mostly ASCII (7bit) and uses some of the spare bits for argument indication (sentinals). This causes some havoc with trying to support non-Latin languages as we ideally need to move to UTF-8. The arguments vary from 1-4 bytes of information and crucially they might have 0 as one of those bytes. This potential 0 means that all C-string functions strcat, strcpy, strlen may work incorrectly and not understand the string. Therefore we now have locoStrlen, locoStrcpy to replace them. Eventually we will require to rework all string handling so that arguments are safely encoded in (hopefully) UTF-8 but that is a long way off.

# OpenLoco version 22.10 (2022-10-09)

## Change Company Face Game Command [#1656]
Changing the company face is much more complex than you might think. Each company face is a seperate Object and changing the face requires unloading and loading objects. This is something that CS generally avoids and limits to the scenario editor. It gives key insight as to how to enable the object selection window in sandbox play and how best to network the object selection window in the future.

## Change Company Name Game Commands [#1640]
These game commands are pretty self explanitory. The implementation allowed for fixing some known buffer overflows in the original code. These were the last change name game commands to implement.

## Industry Manager Functions [#1605, #1621]
Implement various Industry Manager functions. These functions were used at the end of each month to build new industries if the game decided that there was not enough. It has helped to expose a large amount of the flags that an Industry Object can have and the information will be of use to any modders or min/maxers. IndustryManager::findRandomNewIndustryLocation is the key function for this.

## Refactors
A number of code style refactors and cleanups to help unify the codebase. There are luckily not many code style issues in the codebase anymore.

## Build
CMake is a build system that is the defacto standard in C++. We are slowly moving OpenLoco to being a purely CMake project and removing the MS .sln file. This will help unify the build experience across Windows and Linux. As an added bonus it will mean Linux devs do not need to understand the MS .vcxproj file format to be able to add new files or move files around. There is still further work to do though before removal of the .sln file.

## Misc
Added a development log (this). Most items in the project will be covered in the changelog but for items that do not normally warrant a changelog entry they will be captured under this log.

