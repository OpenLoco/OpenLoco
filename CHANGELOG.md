18.02+ (in development)
------------------------------------------------------------------------
- Feature: [#163] Remove terraforming limits outside of scenario editor.
- Feature: [#212] Add fullscreen support.
- Fix: Tooltips were calling the wrong event.

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
