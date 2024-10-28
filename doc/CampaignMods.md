# Warzone 2100 Campaign Mods

In version 4.5.0, a revised process for packaging, distributing, and loading campaign mods was introduced.

- [Basic Structure](#basic-structure)
- [mod-info.json](#mod-infojson)
  - [Tweak Options](#tweak-options)
    - [camTweakOptions](#camtweakoptions)
    - [customTweakOptions](#customtweakoptions)
    - [Accessing Tweak Options from scripts](#accessing-tweak-options-from-scripts)
  - [Example JSON](#example-json)
- [Installing a Campaign Mod](#installing-a-campaign-mod)

# Basic Structure

A campaign mod is a mod `.wz` file (zip archive, renamed) that contains either:
- stats modifications to the base campaign (a "campaign balance mod") \
or
- an entirely new / alternate campaign

In its root, it must contain a [`mod-info.json`](#mod-infojson) file with details as specified below.

It may also optionally contain a `mod-banner.png` image, which is used as the backdrop for the hero / title area of the campaign selector. (Recommended size: approximately 480 × 200. May be stretched to fit.)

# mod-info.json

- **name**: (string) A descriptive name for your mod
- **version**: (string) A version number for your mod (semver format recommended: major.minor.patch)
- **author**: (string) The author's name
- **type**: (string) The type of the mod
  - Options are:
    - `"alternateCampaign"`
    - `"campaignBalanceMod"`
- **license**: (string) An SPDX License Identifier string, specifying the license for the mod (examples: `"CC0-1.0"`, `"GPL-2.0-or-later"`, `"CC-BY-4.0 OR GPL-2.0-or-later"`)
- **minVersionSupported**: (string) The minimum version of Warzone 2100 the mod supports (example: `"4.5.0"`)
- **maxVersionTested**: (string) The maximum version of Warzone 2100 that the mod has been tested with
- **description**: [(JSON Localized String)](WZJsonLocalizedString.md) A description of the mod
- updatesURL: (string) A URL to which a user can browse to obtain a newer version of your mod, if available
  - We recommend hosting your mod on GitHub, and using GitHub releases. ex: `"https://github.com/<USER>/<MODREPO>/releases/latest"`
- **campaigns**: (array) An array of campaign json file names (which will be loaded from `campaigns/<filename>` in your mod)
  - Note: Required for alternateCampaigns. Will be ignored for campaignBalanceMods.
- **universe**: (string) The "universe" in which your campaign's story takes place
  - Options are:
    - `wz2100` (usually used for campaign balance mods that just provide stats modifications to the "official" Warzone 2100 story)
    - `wz2100extended` (for mods set in the extended Warzone 2100 universe - for example, by extending the campaign or providing additional content or stories)
    - `unique` (for alternate campaigns set in their own unique universe / storyline, disconnected from the Warzone 2100 universe)
- **difficultyLevels**: The difficulty levels supported by your alternate campaign
  - This can either be:
    - (string) `"default"` (allow user to select from all difficulty levels)
      - This is the recommended option unless you (for some reason) want to limit the difficulty options available
    - (array) An array containing one or more of the following difficulty options: [`"super easy"`, `"easy"`, `"normal"`, `"hard"`, `"insane"`]
- camTweakOptions: (object) [See Tweak Options](#tweak-options)
- customTweakOptions: (object) [See Tweak Options](#tweak-options)

## Tweak Options

Customizable options can be suppled in the `mod-info.json` file which are:
1. Displayed to the user in the campaign selector screen, and toggleable by the user
2. Passed in the `tweakOptions` global to the scripts (so you can alter behavior based on these options)

### camTweakOptions

There are several built-in options, which can be supplied in the `camTweakOptions` object as follows:

```json
  "camTweakOptions": [
    {
      "id": "timerPowerBonus",
      "default": false,
      "userEditable": true
    },
    {
      "id": "classicTimers",
      "default": false,
      "userEditable": true
    },
    {
      "id": "playerUnitCap40",
      "default": false,
      "userEditable": true
    },
    {
      "id": "autosavesOnly",
      "default": false,
      "userEditable": true
    }
  ]
```

These ids correspond to options that are handled by the core game + base campaign script.

If you are making a campaign balance mod, you can and should supply this list so that users can configure these options.
(You may want to adjust the default values.)

### customTweakOptions

You may also supply one or more entirely custom options, with your own localized name and details.

Example:
```json
  "customTweakOptions": [
    {
      "id": "modname_customoption1",
      "type": "bool",
      "default": true,
      "displayName": {
        "en": "Short name of tweak"
      },
      "description": {
        "en": "A more detailed description of what the option controls / does"
      }
    }
  ]
```

- **id**: (string) A unique id for the tweak option
  - Recommendation: Use a prefix in the "id" value that's related to your mod, to avoid any naming conflicts
- **type** (string) Must be `"bool"`
- **default**: The default value for the option
- **displayName**: [(JSON Localized String)](WZJsonLocalizedString.md) A short name for the tweak
- **description**: [(JSON Localized String)](WZJsonLocalizedString.md) A more detailed description of what the option controls / does

### Accessing Tweak Options from scripts

The `tweakOptions` global variable is available to scripts, and contains a key for each "id" which is set to its value.

Example:
```js
tweakOptions['timerPowerBonus']; // will be `true` or `false` based on the user's configuration
```

## Example JSON:

An example `mod-info.json` for an alternate campaign:

```json
{
  "name": "ModName",
  "version": "1.0.1",
  "author": "AuthorName",
  "type": "alternateCampaign",
  "license": "CC0-1.0",
  "minVersionSupported": "4.5.0",
  "maxVersionTested": "4.5.0",
  "description": {
     "en": "Set out under the command of renowned General Edmund Clayde to lay the foundations of a new world. (Prequel)",
     "de": "<german translation of the description would go here>"
  },
  "updatesUrl": "https://github.com/past-due/ModName/releases/latest",
  "campaigns": [
    "mycampaign.json"
  ],
  "universe": "wz2100extended",
  "difficultyLevels": "default",
  "camTweakOptions": [
    {
      "id": "timerPowerBonus",
      "default": false,
      "userEditable": true
    },
    {
      "id": "classicTimers",
      "default": false,
      "userEditable": true
    },
    {
      "id": "playerUnitCap40",
      "default": false,
      "userEditable": true
    },
    {
      "id": "autosavesOnly",
      "default": false,
      "userEditable": true
    }
  ],
  "customTweakOptions": [
    {
      "id": "modname_customoption1",
      "type": "bool",
      "default": true,
      "displayName": {
        "en": "Short name of tweak"
      },
      "description": {
        "en": "A more detailed description of what the option controls / does"
      }
    }
  ]
}
```

# Installing a Campaign Mod

Campaign mods can be placed in the `<config dir>/mods/campaign` directory, in Warzone 2100 v4.5.0 or later.
