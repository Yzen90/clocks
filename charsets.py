localizations = {"latin": ["en-US.json", "es-MX.json"]}

for charset, files in localizations.items():
    characters = "0123456789%×"

    for l10n in files:
        with open("i18n/" + l10n, "r", encoding="utf-8") as file:
            characters = characters.join(file.read())

    characters = "".join(sorted(set(characters)))

    with open("build/" + charset + ".charset", "w", encoding="utf-8") as file:
        file.write(characters)
