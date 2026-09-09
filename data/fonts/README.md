# Optional Hangul fonts

By default OpenLoco does **not** ship font files. Korean Hangul is drawn with
`stb_truetype` from fonts already installed on the system:

| Platform | Tried fonts |
| --- | --- |
| Windows | Malgun Gothic (`malgunbd.ttf` / `malgun.ttf`) |
| macOS | Apple SD Gothic Neo |
| Linux | Nanum Gothic |

## Optional: Galmuri (pixel Hangul)

For clearer Hangul at the original 8/10/15px sprite-font sizes, you may drop
[Galmuri](https://github.com/quiple/galmuri) (SIL OFL 1.1) into this folder:

- `Galmuri7.ttf` (8px)
- `Galmuri9.ttf` (10px)
- `Galmuri14.ttf` (15px)

If present, OpenLoco loads them before the system Hangul fonts. Bundling them
in the official distribution is optional and left for maintainers to decide.
