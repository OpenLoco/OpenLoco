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

