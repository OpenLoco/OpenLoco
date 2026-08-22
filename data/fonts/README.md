# Bundled Unicode fonts

Used by OpenLoco to draw Hangul, kana, and Han at sprite-font size.

| File | Script | License |
| --- | --- | --- |
| `Galmuri7.ttf` / `Galmuri9.ttf` / `Galmuri14.ttf` | Hangul, kana, JP/KR Han (pixel, 8/10/15px). Not used for Simplified Chinese. | SIL OFL 1.1, Lee Minseo (`Galmuri-LICENSE.txt`) |
| `A2Z-Bold.ttf` | Hangul fallback (에이투지체 Bold) | SIL OFL 1.1, Autonomous A2Z / Freesentation |
| `NotoSansCJKjp-Regular.otf` | Japanese | SIL OFL 1.1, Google / Adobe (`NotoCJK-OFL.txt`) |
| `NotoSansCJKsc-Regular.otf` | Simplified Chinese | SIL OFL 1.1, Google / Adobe (`NotoCJK-OFL.txt`) |

Galmuri is rendered at its native pixel size so Hangul stays on-grid. System fonts are still tried as fallbacks.
