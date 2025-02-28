from fontTools.merge import Merger

from shared.fonts import trim_with_custom_codepoints


trim_with_custom_codepoints("resources/NotoSansJP-Regular.ttf", "build/ja-JP.ttf", "日本語", ord('\uE000'))

trim_with_custom_codepoints("resources/NotoSansSC-Regular.ttf", "build/zh-CN.ttf", "简体中文", ord('\uE010'))


merger = Merger()
merger.merge(["build/ja-JP.ttf", "build/zh-CN.ttf"]).save("assets/support.ttf")
