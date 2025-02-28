from contextlib import contextmanager
from fontTools.ttLib import TTFont
from fontTools.subset import Subsetter, Options


class ReverseDictView:
  def __init__(self, original_dict):
    self.original_dict = original_dict

  def __getitem__(self, key):
    for k, v in self.original_dict.items():
      if v == key:
        return k
    raise KeyError(f"{key} not found in the reversed dictionary")

  def __contains__(self, key):
    return key in self.original_dict.values()

  def items(self):
    for k, v in self.original_dict.items():
      yield v, k


@contextmanager
def font_cmap(font: TTFont):
  yield font.getBestCmap()


@contextmanager
def reverse_cmap(font: TTFont):
  yield ReverseDictView(font.getBestCmap())


def trim_with_custom_codepoints(font_path: str, output_path: str, to_keep: str, custom_codepoint_start: int):
  with TTFont(font_path) as font:
    codepoints = [ord(char) for char in to_keep]

    start = ord('\uE000')
    custom_codepoints = [codepoint for codepoint in range(
        custom_codepoint_start, custom_codepoint_start + len(codepoints))]

    with font_cmap(font) as cmap:
      for codepoint, custom in zip(codepoints, custom_codepoints):
        cmap[custom] = cmap[codepoint]

    options = Options()
    options.glyph_names = True
    options.symbol_cmap = True
    options.layout_features = []
    options.ignore_missing_glyphs = False
    options.ignore_missing_unicodes = False

    subsetter = Subsetter(options)
    subsetter.populate(unicodes=custom_codepoints)
    subsetter.subset(font)

    font.save(output_path)
    font.close()
