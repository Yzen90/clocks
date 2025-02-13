import logging

from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options

from shared.fonts import reverse_cmap


glyphs = [
    "brightness_auto",
    "brightness_high",
    "dark_mode",
    "night_sight_auto",
    "routine",
]

font = TTFont("resources/MaterialSymbolsRounded_Filled-Regular.ttf")

codepoints = {}
with reverse_cmap(font) as reversed_cmap:
    for glyph in glyphs:
        if glyph in reversed_cmap:
            codepoints[glyph] = reversed_cmap[glyph]
        else:
            logging.warning(f'Codepoints: Glyph "{glyph}" not found.')


options = Options()
options.glyph_names = True
options.symbol_cmap = True
""" options.legacy_cmap = True
options.layout_features = ["*"]
options.notdef_glyph = True
options.recommended_glyphs = True """
options.ignore_missing_glyphs = False
options.ignore_missing_unicodes = False
options.verbose = True


subsetter = Subsetter(options)

subsetter.populate(glyphs=glyphs, unicodes=codepoints.values())
subsetter.subset(font)

font.save("assets/icons.ttf")
font.close()


print(codepoints)

""" print(f"{codepoints[glyph]:04X}")"""
