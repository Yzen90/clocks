import logging

from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options

from shared.fonts import *


glyphs = [
    "brightness_auto",
    "brightness_high",
    "dark_mode",
    "night_sight_auto",
    "routine",
]

font = TTFont("resources/MaterialSymbolsRounded_Filled-Regular.ttf")

glyph_ids = []
codepoints = {}
with reverse_cmap(font) as reversed_cmap:
    font_glyphs = font["glyf"]

    for glyph in glyphs:
        if glyph in reversed_cmap:
            codepoints[glyph] = reversed_cmap[glyph]
            glyph_ids.append(font.getGlyphID(glyph))

            fill_glyph = glyph + ".fill"
            if fill_glyph in font_glyphs:
                font_glyphs[glyph] = font_glyphs[fill_glyph]

        else:
            logging.warning(f'Codepoints: Glyph "{glyph}" not found.')


options = Options()
options.glyph_names = True
options.symbol_cmap = True
options.notdef_glyph = False
options.layout_features = []
options.ignore_missing_glyphs = False
options.ignore_missing_unicodes = False


subsetter = Subsetter(options)

subsetter.populate(gids=glyph_ids)
subsetter.subset(font)


font.save("assets/icons.ttf")
font.close()


""" print(f"{codepoints[glyph]:04X}")"""
