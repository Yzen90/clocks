from contextlib import contextmanager
from fontTools.ttLib import TTFont


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
def cmap(font: TTFont):
    yield font.getBestCmap()


@contextmanager
def reverse_cmap(font: TTFont):
    yield ReverseDictView(font.getBestCmap())
