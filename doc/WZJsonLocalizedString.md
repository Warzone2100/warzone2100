# WZ JSON Localized String

Warzone 2100 supports specifying localized strings in certain JSON files.

Anything that's listed as `(JSON Localized String)` may either be:

- A (string) value
  - Example:
  ```json
  "description": "A description of the mod in English, and only English."
  ```
- _Verbose Form:_ An (object) with a key for each language code and a (string) value that is the translation
  - Example:
  ```json
  "description": {
    "en": "A description of the mod in English."
    "de": "The same description, but in German."
  }
  ```
  - Note:
    - The base / default language for the game is English (`"en"`). We strongly recommend that you _always_ provide an `en` translation of all localized strings.
    - The language codes correspond to the ones supported by the game (see the `.po` files in: https://github.com/Warzone2100/warzone2100/tree/master/po)

## Recommendations:

- For JSON Localized String values in files bundled with the game:
  - These should generally just be normal JSON string values _in WZ's "base" language (English)_
  - They will automatically be included in the message catalogs, and translations will be managed via Crowdin
- For values in files in a mod which is distributed separately:
  - If you want to include your own bundled translations for these strings, you can optionally use the verbose form of a JSON Localized String (the object form, which contains keys/values for different languages)
