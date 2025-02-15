import logging
import json
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options

from shared.fonts import *


glyphs = json.loads(Path("icons.json").read_text("utf_8"))
codepoints = {}

with TTFont("resources/MaterialSymbolsRounded_Filled-Regular.ttf") as font:
    with reverse_cmap(font) as reversed_cmap:
        font_glyphs = font["glyf"]

        for glyph in glyphs:
            if glyph in reversed_cmap:
                codepoints[glyph.upper()] = reversed_cmap[glyph]

                fill_glyph = glyph + ".fill"
                if fill_glyph in font_glyphs:
                    font_glyphs[glyph] = font_glyphs[fill_glyph]

            else:
                logging.warning(f'Icons: Codepoint for glyph "{glyph}" not found.')

    options = Options()
    options.glyph_names = True
    options.symbol_cmap = True
    options.layout_features = []
    options.ignore_missing_glyphs = False
    options.ignore_missing_unicodes = False

    subsetter = Subsetter(options)
    subsetter.populate(glyphs)
    subsetter.subset(font)

    font.save("assets/icons.ttf")
    font.close()


cpp = """#include <string>

using std::string;

namespace material_symbols {\n
"""

for icon, codepoint in codepoints.items():
    cpp += "  constexpr const string " + icon + f' = "\\u{codepoint:04X}";\n'

cpp += "\n}\n"

with open("src/ui/icons.hpp", "w", encoding="utf-8") as file:
    file.write(cpp)
