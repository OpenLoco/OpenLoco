23.06.1+ (???)
------------------------------------------------------------------------

23.06.1 (2023-06-17)
------------------------------------------------------------------------
- Fix: [#1970] Water waves don't animate.
- Fix: [#1992] Crash when the AI uses vehicle refit command.
- Technical: [#1982] Performance improvements for file I/O and general cleanup of the stream interfaces.

23.06 (2023-06-12)
------------------------------------------------------------------------
- Feature: [#376] Allow fractional UI scaling in addition to integer scaling.
- Feature: [#418] Use hardware-backed SDL canvas when available for better performance.
- Feature: [#1963] Make news sound effects optional through message settings window.
- Feature: [#1973] Allow reversing a vehice's route order table.
- Fix: [#1966] Unable to select terrain type for terrain painting.
- Fix: [#1968] Crash when game tries to plant trees during natural growth.

23.05 (2023-05-27)
------------------------------------------------------------------------
- Fix: [#1281] The construction window won't open under some circumstances.
- Fix: [#1942] Crash when attempting to clear a building with the clear construction tool.
- Fix: [#1946] Crash when loading openloco.yml config that has no shortcuts defined.
- Fix: [#1953] Cloning a vehicle doesn't copy cargo retrofit selection.
- Technical: [#1934] The performance of scrollable lists has been improved.

23.04.1 (2023-04-27)
------------------------------------------------------------------------
- Feature: [#1934] Present a list of objects that failed to load, instead of just the last one.
- Fix: [#1925] Not all custom objects are being exported into save files as expected.
- Fix: [#1929] Vehicles report local/express status incorrectly.

23.04 (2023-04-23)
------------------------------------------------------------------------
- Fix: [#1907] Hooks fail to install on ARM macOS with wine.
- Change: [#1908] Detects if terminal is VT100 capable and uses colors for the output, can be disabled using [NO_COLOR](https://no-color.org/).

23.03 (2023-03-16)
------------------------------------------------------------------------
- Fix: [#1021] Excessive CPU usage when the Load/Save window is open.
- Fix: [#1783] Crash when drawing track tunnels under certain situations.
- Fix: [#1869] Crash when changing language.
- Fix: [#1875] Tooltips weren't line-wrapping properly.
- Fix: [#1882] Process hang when the game trys to crash uncleanly.
- Fix: [#1887] Autosave frequency dropdown not working.
- Fix: [#1888] Tree shadows disappeared and docks double displayed.
- Fix: [#1890] Crash when wrapping words that are longer than max width.

23.02 (2023-02-19)
------------------------------------------------------------------------
- Feature: [#1837]: Add search/filter functionality to object selection window.
- Fix: [#1475] Slow view panning with uncaps FPS disabled.
- Fix: [#1763] Title music does not stop when unchecked in options window.
- Fix: [#1772] Toggling edge scrolling option does not work.
- Fix: [#1798] Memory leak when resizing the window.
- Fix: [#1842] Track, Road and Dock objects incorrectly unloaded causing packing issues.
- Fix: [#1853] Company list not sorted properly by status.
- Change: [#1823] Prevent edge scroll if the window has no input focus.

23.01 (2023-01-25)
------------------------------------------------------------------------
- Feature: [#1745] Add cheat to instantly win any scenario/challenge.
- Fix: [#528] Snow rendering issue with Steam provided Locomotion.
- Fix: [#1750] Scenario index is not updated when in-game language changes.
- Change: [#1698] The prompts asking to locate the Locomotion install folder have been improved.

22.12 (2022-12-22)
------------------------------------------------------------------------
- Feature: [#175, #938] Allow modifying object selection in-game (cheat menu).
- Feature: [#1664] Allow modifying scenario options in-game (cheat menu).
- Feature: [#1736] Allow disabling town renewal/expansion/growth (options menu).
- Fix: [#1727] Starting loan is not displayed properly in scenario options.
- Fix: [#1737] Unpacked objects accidentally get installed into wrong folder.
- Fix: [#1738] Last selected Misc building not remembered in Building Placement tab.
- Change: [#1736] The miscellaneous options tab has been redesigned to reduce clutter.

22.11 (2022-11-20)
------------------------------------------------------------------------
- Feature: [#1696] Display filename in object selection window.
- Fix: [#1549] Objects do not load localised strings.
- Fix: [#1676] Crash when viewing newly created scenarios in scenario list.
- Fix: [#1680] Title music plays in scenario editor.
- Fix: [#1711] Crash on linux when config folder for OpenLoco is not manually created.
- Fix: [#1720] Crash when indexing object files that fail validation.
- Fix: [#1720] Object selection not properly checking valid object selection.

22.10 (2022-10-09)
------------------------------------------------------------------------
- Feature: [#1608] Added character limit label in text input windows.
- Feature: [#1666] Allow saving giant (full map) screenshots.
- Change: [#1623] Title screen music volume is now bound to the music volume setting.
- Fix: [#1237] Long entity (company) names may be cut-off incorrectly.
- Fix: [#1650] Acquire all company assets cheat may cause some trains and trams to crash.

22.09 (2022-09-04)
------------------------------------------------------------------------
- Fix: [#1612] Ambient audio no longer playing.
- Fix: [#1613] Crash when viewing the Build Trains window with certain trains.
- Fix: [#1614] Crash when ai picks up none existent vehicles.

22.08 (2022-08-25)
------------------------------------------------------------------------
- Fix: [#729] Audio volume not initialised to config volume.
- Fix: [#1557] Vehicle capacity texts are corrupted.
- Fix: [#1578] Clang-compiled openloco does not list scenarios.
- Fix: [#1583] Newspaper text line height is wrong.
- Technical: [#1565] Any missing objects are now listed in the dev console when encountered.
- Technical: [#1600] Allow running OpenLoco through Wine on Apple Silicon Macs

22.06.1 (2022-07-01)
------------------------------------------------------------------------
- Fix: [#1552] Crash on startup on release builds.

22.06 (2022-06-30)
------------------------------------------------------------------------
- Feature: [#1273] Added option to toggle visibility of cash pop-ups.
- Feature: [#1433] For tracked vehicles, show the length in the vehicle details window and object selection.
- Feature: [#1543] For multi-car vehicles, show the car count in vehicle details window.
- Fix: [#1510] No music in game.

22.05.1 (2022-05-17)
------------------------------------------------------------------------
- Fix: [#1504] Trains can get stuck on slopes under certain conditions causing a build up of negative speed.
- Fix: [#1505] Graphical glitch at edge of map making void have highest precedence when drawing.
- Fix: [#1508] Immediate failure of scenarios due to incorrect setting of time limit.

22.05 (2022-05-08)
------------------------------------------------------------------------
- Feature: [#1457] Added option to invert right-mouse panning the game view.
- Feature: [#1484] Hold shift when placing a vehicle to start it immediately.
- Fix: [#293] Menu screen and other window corruption when many objects loaded.
- Fix: [#1463] Crash when opening the build window under certain situations.
- Fix: [#1499] Trees are missing their wilt effects.
- Fix: [#1499] Certain trees display the wrong image for certain seasons.

22.04 (2022-04-12)
------------------------------------------------------------------------
- Feature: [#1316] Added a cheat to allow building locked vehicles.
- Feature: [#1435] Separate landscape height tool from landscape paint tool.
- Fix: [#1229] Reliability cheat would not apply to multi power locos.
- Fix: [#1241] Total vehicle power formatted incorrectly.
- Fix: [#1242] Cannot select both sides of a single way signals.
- Fix: [#1250] Cannot save landscape in scenario editor.
- Fix: [#1279] Various crashes when removing vehicles.
- Fix: [#1284] Delete key doesn't work in text input widgets.
- Fix: [#1379] Year formatted correctly in date cheat window.
- Fix: [#1393] Vehicles not sorted correctly in build vehicle window.
- Fix: [#1400] Imperial-to-metric power conversion didn't match vanilla.
- Fix: [#1424] Crash when loading a scenario after waiting on title screen.
- Fix: [#1434] Can set 0 or more than 80 towns in scenario editor.
- Fix: [#1449] Autopay tooltip was formatted poorly.
- Fix: [#1450] Some strings were erroneously referenced in the language files.
- Fix: [#1453] Vehicle modify button image disappears for certain company colours.

22.03.1 (2022-03-08)
------------------------------------------------------------------------
- Fix: [#1375] All vehicles are locked.

22.03 (2022-03-08)
------------------------------------------------------------------------
- Feature: [#1327] Readd the game intro (use commandline switch --intro to enable).
- Feature: [#1350] Show vehicle obsolete date in build vehicle window.
- Feature: [#1354] Added a cheat to display locked vehicles.
- Fix: [#239] Vehicles sound do not modify pitch and incorrect sounds can be loaded when loading saves.
- Fix: [#1280] Crash when removing crashed vehicles with news window open.
- Fix: [#1320] Inability to mark scenario as complete.
- Fix: [#1323] Playlist crash when setting the date really far into the future.
- Fix: [#1325] Crash when saving second loaded scenario of a playthrough.
- Fix: [#1328] Various object loading bugs related to custom object files causing crashes on load.
- Change: [#1276] Transfering cargo is now viable. The cargo age is calculated as the weighted average of the present and delivered cargo.
- Technical: [#1347] Now using OpenAL-soft engine for audio, replacing SDL2_mixer.

22.02 (2022-02-06)
------------------------------------------------------------------------
- Feature: [#1271] Vehicles display their total capacity in the cargo window.
- Feature: [#1278] Added a cheat to change the in-game date.
- Feature: [#1284] Added automatic loan repayment option in company finances window.
- Feature: [#1291] Modified the time panel date format to display day.
- Feature: [#1310] Added keyboard shortcuts for changing the game simulation speed.
- Fix: [#1236] Incorrect cargo capacity displayed for vehicle objects.

21.10 (2021-10-17)
------------------------------------------------------------------------
- Feature: [#1170] Add option to disable train reversing at signals.
- Fix: [#788] Opening windows larger than the game window might cause a crash.
- Fix: [#1166] Incorrect surface selection when over water.
- Fix: [#1176] Headquarters ghost destroys buildings.
- Fix: [#1178] Game does not start when no legacy config file is present.
- Fix: [#1179] Default shortcut keys are not assigned when no config file is present.
- Fix: [#1190] Terraform window's area increase/decrease buttons are not holdable.
- Fix: [#1218] Crash when saving landscapes or scenarios in the scenario editor.
- Fix: [#1222] Unable to right click interact with road stations built on neutral or ai created road.

21.09.1 (2021-09-14)
------------------------------------------------------------------------
- Fix: [#1161] Stuttering vehicle movement due to incorrect vehicle positions.
- Fix: [#1162] Signal side placement is inverted.
- Fix: [#1162] Single side signal removal incorrectly removing both signals.
- Fix: [#1162] Cursor hot spots are incorrect for some cursors.
- Fix: [#1167] Incorrect right click interact with some road pieces.
- Fix: [#1171] Vehicles incorrectly placed underground on load.
- Fix: [#1173] Turnaround track piece not selectable on right hand drive scenarios.

21.09 (2021-09-11)
------------------------------------------------------------------------
- Feature: [#784] Optional keyboard shortcuts for construction window.
- Fix: [#1108] Road selection not being remembered.
- Fix: [#1123] Right click interaction of road/tram causing crashes or money.
- Fix: [#1124] Confirmation prompt captions are not rendered correctly.
- Fix: [#1127] Crash during vehicle renewal cheat.
- Fix: [#1144] Options window spawns behind title logo.
- Fix: [#1146] Tram turnaround piece activates wrong button.
- Change: [#1104] Exceptions now trigger a message box popup, instead of only being written to the console.
- Change: [#1141] The Enter key on the numeric keypad can now be bound separately.

21.08 (2021-08-12)
------------------------------------------------------------------------
- Fix: [#366] Original Bug. People and mail cargo incorrectly delivered to far away stations.
- Fix: [#1035] Incorrect colour selection when building buildings.
- Fix: [#1070] Crash when naming stations after exhausting natural names.
- Fix: [#1094] Repeated clicking on construction window not always working.
- Fix: [#1095] Individual expenses are drawn in red, not just the expenditure sums.
- Fix: [#1102] Invalid file error when clicking empty space in file browser.
- Change: [#298] Planting clusters of trees now costs money and influences ratings outside of editor mode.
- Change: [#1079] Allow rotating buildings in town list by keyboard shortcut.

21.07 (2021-07-18)
------------------------------------------------------------------------
- Feature: [#856] Allow filtering the vehicle list by station or cargo type.
- Fix: [#982] Incorrect rating calculation for cargo causing penalty for fast vehicles.
- Fix: [#984] Unable to reset/regenerate station names by using an empty name.
- Fix: [#1008] Inability to decrease max altitude for trees in landscape editor.
- Fix: [#1016] Incorrect detection of station causing incorrect smoke sounds.
- Fix: [#1044] Incorrect rotation of headquarters when placing. No scaffolding when placing headquarters.
- Technical: [#986] Stack misalignment in GCC builds caused unexplained crashes on Linux and Mac during interop hooks with loco.exe.
- Technical: [#993] Retry hook installation to fix incompatibles with older wine versions.
- Technical: [#1006] Add breakpad-based dumping for MSVC builds

21.05 (2021-05-11)
------------------------------------------------------------------------
- Feature: [#184] Implement cheats window with financial, company, vehicle, and town cheats.
- Feature: [#857] Remember last save directory in configuration variable.
- Feature: [#923] Tween (linear interpolate) entities when frame limiter is uncapped for smoother movement.
- Fix: [#914] Boats get stuck in approaching dock mode when water is above a certain height. This was incorrectly fixed in 21.04.1.
- Fix: [#923] Decouple viewport updates from game ticks for smoother panning and zooming.
- Fix: [#927] Some available industries are missing in the 'Fund new industries' tab.
- Fix: [#945] Station construction preview image is using wrong colours.
- Fix: [#957] Element name is not shown when inspecting track elements using the Tile Inspector.
- Change: [#975] The multiplayer toggle button on the title screen has been hidden, as multiplayer has not been reimplemented yet.

21.04.1 (2021-04-14)
------------------------------------------------------------------------
- Fix: [#914] Boats get stuck in approaching dock mode when water is above a certain height.
- Fix: [#915] Money subtractions with large values incorrectly calculated causing negative money.

21.04 (2021-04-10)
------------------------------------------------------------------------
- Feature: [#451] Optionally show an FPS counter at the top of the screen.
- Feature: [#831] Add a tile inspector, allowing inspection of tile element data (read-only).
- Feature: [#853] Allow unlocking FPS by detaching game logic from rendering.
- Fix: [#391] Access violation in windows after exiting games.
- Fix: [#804] Enter key not confirming save prompt.
- Fix: [#809] Audio calculation not using the z axis.
- Fix: [#825] Potential crash when opening town rename prompt.
- Fix: [#838] Escape key doesn't work in confirmation windows.
- Fix: [#845] Town growth incorrectly calculated causing more aggressive growth than should be possible.
- Fix: [#853] The game run slightly, but noticeably, slower than vanilla Locomotion.
- Fix: [#860] Incorrect capacity information for vehicles that do not carry cargo (e.g. is a train engine).

21.03 (2021-03-06)
------------------------------------------------------------------------
- Feature: [#125] Allow construction while paused using a new optional cheats/debugging menu.
- Feature: [#796] Allow users to toggle sandbox mode in-game using the cheats menu.
- Fix: [#294] Crash when setting company name twice.
- Fix: [#697] Ghost elements are not removed in autosaves.
- Fix: [#794] Game does not stay paused while in construction mode.
- Fix: [#798] Setting waypoints on multitile track/road elements corrupts the position.
- Fix: [#801] Initial save path does not contain a trailing slash.
- Fix: [#807] Incorrect vehicle animation for speed based animations like hydrofoils when at max speed.
- Change: [#361] The game now allows scenarios to start from 1800, with adjusted inflation.
- Change: [#787] Scenery and building interaction is now disabled when see-through.

21.02 (2021-02-20)
------------------------------------------------------------------------
- Feature: [#122] Allow vehicles to be cloned from the vehicle window.
- Feature: [#690] Automatically save the game at regular intervals.
- Feature: [#702] Optional new map generator (experimental).
- Fix: [#151] Mouse moves out of window when looking around.
- Fix: [#588] 'Cancel or Show Last Announcement' shortcut doesn't close announcements.
- Fix: [#679] Crash when changing ground texture.
- Fix: [#694] Selecting a song to play is guaranteed to not play it.
- Fix: [#712] Load / save window tries to show preview for item after last.
- Fix: [#721] Incorrect catchment area for airports.
- Fix: [#725] Company value graph does not display correctly.
- Fix: [#744] Rendering issues ('Z-fighting') with vehicles over bridges.
- Fix: [#766] Performance index is off by a factor of 10 in scenario options window.
- Fix: [#769] Waypoints for road vehicles could not be set.
- Fix: [#779] Town list displays the wrong amount of stations.
- Change: [#690] Default saved game directory is now in OpenLoco user directory.
- Change: [#762] The vehicle window now uses buttons for local/express mode.

20.10 (2020-10-25)
------------------------------------------------------------------------
- Feature: [#569] Option/cheat to disable AI companies entirely.
- Fix: [#573] Crash caused by opening Road construction window.
- Fix: [#588] Crash caused by changing default audio device.
- Fix: [#595] Implementation mistake in CreateVehicle could lead to crashes.
- Fix: [#635] Land tool not working properly, due to tool drag events not passing on coordinates.
- Fix: [#648] Fix crash in vehicle update head caused by CreateVehicle.

20.07 (2020-07-26)
------------------------------------------------------------------------
- Feature: [#523] Holding the construction window's build or remove button will keep repeating the action.
- Fix: [#158] Pressing shift to build underground tracks automatically builds ten track pieces.
- Fix: [#390] Load/save window causes a crash when trying to access bad directories.
- Fix: [#397] Opening a tutorial crashes the game.
- Fix: [#485] Incorrect position of exhaust smoke on vehicles.
- Fix: [#491] Station/city name labels are hidden from viewport when saving.
- Fix: [#529] Tree-related industries are not updating properly.
- Fix: [#530] Industry production not starting up under some conditions.
- Removed: Clicking track / road construction while holding shift will place 10 pieces in a row.

20.05.1 (2020-05-30)
------------------------------------------------------------------------
- Fix: [#487] Checkbox behaviour reversed for industry opening/closing in landscape generation options.
- Fix: [#488] Repeated clicking may lead to a negative loan.
- Fix: [#494] Farms not producing grain for stations in Mountain Mayhem scenario.
- Fix: [#498] Clicking newly invented vehicle in news throws out of range exception.

20.05 (2020-05-24)
------------------------------------------------------------------------
- Feature: [#77] Add "Exit OpenLoco" to the main menu.
- Fix: [#264] Option 'Export plug-in objects with saved games' is partially cut off.
- Fix: [#299, #430] Crash due to added null-chars when manually specifying Locomotion directory.
- Fix: [#359] Widgets tied to tools could get stuck in pressed state.
- Fix: [#388] Re-center Options window on scale factor change.
- Fix: [#396] Preferred owner name is not saved.
- Fix: [#409] Incorrect refund cost when deleting signals.
- Fix: [#412] Game crashes after a while on Great Britain & Ireland 1930.
- Fix: [#423] Date in challenge tooltip is incorrect.
- Fix: [#425] Changing resolution in fullscreen mode doesn't work.
- Fix: [#428] Show an error when a vehicle can't be built due to invalid properties. (Original bug.)
- Fix: [#440] Final segment in town population graphs could show no population.
- Fix: [#467] Incorrect scrolling thumbs when leaving the bottom of an auto resizing window.
- Fix: [#478] Crash when opening narrow gauge tab on train purchasing window
- Change: [#420] Disable window scale factor buttons when not applicable.

20.03 (2020-03-23)
------------------------------------------------------------------------
- Feature: [#347] Screenshots are now saved in PNG format.
- Fix: [#226] Zooming to cursor is buggy on bigger maps.
- Fix: [#296] Correctly show challenge progression in save previews.
- Fix: [#297] Menu click sound not played.
- Fix: [#303] Play title music preference is not saved.
- Fix: [#340] Cargo rating is calculated incorrectly in some edge cases.
- Fix: [#349] Building a signal adds money (macOS/Linux only).
- Fix: [#383] Crash in construction window.
- Fix: Strings were not wrapping properly in the file browser window.
- Change: [#380] Make keypad enter work the same as normal enter.

19.03 (2019-03-01)
------------------------------------------------------------------------
- Feature: [#163] Remove terraforming limits outside of scenario editor.
- Feature: [#178] Allow zooming to cursor position instead of viewport centre.
- Feature: [#192] The option window now includes OpenLoco-specific settings.
- Feature: [#203] Support multiple languages by loading text strings from YAML files.
- Feature: [#212] Add fullscreen support.
- Feature: [#221, #236] Implement audio through SDL2 mixer, introducing audio on Linux and macOS.
- Feature: [#237] Allow nearest neighbour scaling the game on integer intervals.
- Feature: [#275] Allow disabling the title screen music.
- Feature: [#279] Use OpenLoco logo for window icon. (Logo created by [Zcooger](https://github.com/Zcooger))
- Fix: Tooltips were calling the wrong event.
- Fix: [#219, #257] Prevent text from being drawn off-screen.
- Change: [#107] Show git branch and short sha1 hash in version info line.
- Change: [#211] Store configuration file as YAML.

18.02 (2018-02-15)
------------------------------------------------------------------------
- Feature: [#12, #14, #50] Support for Linux and macOS.
- Feature: [#20] Support graphics data files from Stream distribution.
- Feature: Allow player to remove roads that are in use.
- Feature: Towns can now always be renamed (As seen in OpenTTD).
- Feature: Vehicle breakdowns can now be disabled (As seen in OpenTTD).
- Feature: Playable in a resizable window.
- Feature: Clicking track / road construction while holding shift will place 10 pieces in a row.
- Change: [#79] Store `game.cfg`, `plugin.dat` and `scores.dat` in:
  - Windows: `%APPDATA%\OpenLoco`
  - Linux: `~/.config/openloco`
  - macOS: `~/Library/Application Support/OpenLoco`
- Change: [#79] Disable file existence and size checks.
