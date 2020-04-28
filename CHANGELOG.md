20.03+ (???)
------------------------------------------------------------------------
- Feature: [#77] Add "Exit OpenLoco" to the main menu.
- Fix: [#264] Option 'Export plug-in objects with saved games' is partially cut off.
- Fix: [#388] Re-center Options window on scale factor change.
- Fix: [#396] Preferred owner name is not saved.
- Fix: [#423] Date in challenge tooltip is incorrect.
- Fix: [#425] Changing resolution in fullscreen mode doesn't work.
- Fix: [#430] Null-chars added when manually specifying Locomotion directory, preventing launch.
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
- Feature: Click (while holding shift) track / road construction will place 10 pieces in a row.
- Change: [#79] Store `game.cfg`, `plugin.dat` and `scores.dat` in:
  - Windows: `%APPDATA%\OpenLoco`
  - Linux: `~/.config/openloco`
  - macOS: `~/Library/Application Support/OpenLoco`
- Change: [#79] Disable file existence and size checks.
