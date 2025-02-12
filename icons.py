from glob import glob
from fontTools.ttLib import TTFont
from fontTools.svgLib import SVGPath
from fontTools.ttLib.ttFont import newTable
from fontTools.ttLib.tables import _g_l_y_f
from fontTools.ttLib.tables._c_m_a_p import CmapSubtable


font = TTFont()

cmap_table = CmapSubtable.newSubtable(4)
cmap_table.platformID = 3
cmap_table.platEncID = 1
cmap_table.language = 0
cmap_table.cmap = {}

font["cmap"] = newTable("cmap")
font["cmap"].tables = [cmap_table]
font["glyf"] = _g_l_y_f.table__g_l_y_f()


codepoint_start = 0xF0000

for i, svg in enumerate(glob("icons/*.svg")):
    path = SVGPath("icons/" + svg)
    glyph = _g_l_y_f.Glyph()
