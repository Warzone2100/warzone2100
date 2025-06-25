# Warzone 2100 Guide Topics JSON

In version 4.5.0, an in-game guide was introduced.

Guide topics can be specified using a JSON schema and a hierarchical file layout.

- [Basic Layout](#basic-layout)
  - [Files](#files)
  - [Topic IDs](#topic-ids)
  - [Categories & Hierarchy](#categories--hierarchy)
  - [Translatable Strings](#translatable-strings)
- [Guide Topics JSON](#guide-topics-json)
  - [Guide Topic Visuals](#guide-topic-visuals)
    - [Stat Visual](#stat-visual)
    - [Droid Template Visual](#droid-template-visual)
    - [Reticule Button Image](#reticule-button-image)
    - [Secondary Unit Order Buttons](#secondary-unit-order-buttons)
  - [Example JSON](#example-json)
- [Displaying Guide Topics](#displaying-guide-topics)

# Basic Layout

## Files

Guide topic JSON files are located underneath the `guidetopics/` folder in WZ's virtual filesystem.

- The base guide topics are in this repo at: [/data/base/guidetopics/](/data/base/guidetopics/)
  - These end up in the `base.wz` inside a `guidetopics/` folder
- A mod can add guide topics in a root `guidetopics/` folder in its package

## Topic IDs

The path to a guide topic underneath the `guidetopics/` folder corresponds to its `id`.

> Example: \
> A guide topic with id `wz2100::general::backstory` would correspond to a JSON file located at `guidetopics/wz2100/general/backstory.json`

## Categories & Hierarchy

Hierarchy can be established by making any folder into an official level / category via an `_index.json` category guide topic.

This special file will establish a new level of hierarchy in the topics display, and is expected to have a guide topic id that matches the folder path.
> i.e. A category guide topic of `wz2100::general` would correspond to a category JSON file located at `guidetopics/wz2100/general/_index.json`

Category guide topics are automatically loaded when a contained topic is loaded.

## Translatable Strings

Several fields of the Guide Topic JSON are [WZ JSON Localized Strings](WZJsonLocalizedString.md).
- For guide topics bundled with the game:
  - These should generally just be normal JSON string values _in WZ's "base" language (English)_
  - They will automatically be included in the `warzone2100_guide.pot`, and translations will be managed via Crowdin
- For guide topics bundled with a mod:
  - If you want to include your own bundled translations for these strings, you can optionally use the verbose form of a WzJsonLocalizedString (an object which contains keys/values for different languages)


# Guide Topics JSON

- **type**: (string) Should be `"wz2100.guidetopic.v1"`
- **id**: (string) The id for the guide topic. See: [Topic IDs](#topic-ids)
- **title**: [(JSON Localized String)](WZJsonLocalizedString.md) The title for the guide topic
  - See [Translatable Strings](#translatable-strings) for pointers
- version: (string, optional) An optional version string, which can be supplied if a guide topic changes substantially (default is "0")
  -  If the `version` does not match the version of the topic that was viewed by the player, the topic will be treated as "new" until the current `version` of the topic is read
- sort: (string, optional) A string sort key used to sort the guide topic lexicographically relative to others
  - Topics with `sort` specified are always sorted before topics that do not have `sort` specified
- applicableModes: (list, optional) If specified, the specific game mode(s) for which this guide topic will be available
  - Options are:
    - `"campaign"`
    - `"challenge"`
    - `"skirmish"`
    - `"multiplayer"`
- visual: One or more visuals to be displayed below the title of the guide topic
   - See [Guide Topic Visuals](#guide-topic-visuals) for more information
- **contents**: (list of [JSON Localized String](WZJsonLocalizedString.md)s)
  - The contents of the guide topic
  - See [Translatable Strings](#translatable-strings) for pointers
  - For ease of translation, each line should be a separate entry in the `contents` list
    - Tip: Consider splitting long paragraphs into two or more chunks - see the built-in topic examples: [/data/base/guidetopics/wz2100/](/data/base/guidetopics/wz2100/)
  - An empty string entry (`""`) can be used to signify a blank line
- **links**: (list of strings) A list of guide topic `id`s, which will be displayed in a "See Also:" section at the bottom of this guide topic

## Guide Topic Visuals

A guide topic visual is a JSON object that can take the form of one or more different types.

(You can also specify multiple visuals by using an array of these objects. [See an example](/data/base/guidetopics/wz2100/structures/modules.json).)

### Stat Visual

Specify a **`"stat"`** key, with a value of either:
  - (string) The stat id (ex. `"A0CommandCentre"`)
  - (object) An object containing the stat `"id"`, and (optionally `"onlyIfKnown"`)
    - **id**: (string) The stat id (ex. `"A0CommandCentre"`)
    - onlyIfKnown: Optionally limit this visual to being displayed
      - Options are:
        - `"false"` - not limited (default)
        - `"true"` - only show this stat visual if known to the player
        - `"campaign"` - only show this stat visual if known to the player (in campaign mode, otherwise always show)

### Droid Template Visual

Specify a **`"droidTemplate"`** key, with a value of:
  - An object that matches what you'd find in a `templates.json`
  - See example in: [/data/base/guidetopics/wz2100/units/commanders/_index.json](/data/base/guidetopics/wz2100/units/commanders/_index.json)

### Reticule Button Image

Specify a **`"retButId"`** key, with a value of:
  - `"CANCEL"`, `"FACTORY"`, `"RESEARCH"`, `"BUILD"`, `"DESIGN"`, `"INTELMAP"`, or `"COMMAND"`

### Secondary Unit Order Buttons

Specify a **`"secondaryUnitOrderButtons"`** key, with a value of:
  - `"ATTACK_RANGE"`, `"REPAIR_LEVEL"`, `"ATTACK_LEVEL"`, `"ASSIGN_PRODUCTION"`, `"ASSIGN_CYBORG_PRODUCTION"`, `"CLEAR_PRODUCTION"`, `"RECYCLE"`, `"PATROL"`, `"HALTTYPE"`, `"RETURN_TO_LOC"`, `"FIRE_DESIGNATOR"`, `"ASSIGN_VTOL_PRODUCTION"`, `"CIRCLE"`, or `"ACCEPT_RETREP"`

## Example JSON:

Some example guide topics:
- A category topic: [wz2100/general/_index.json](/data/base/guidetopics/wz2100/general/_index.json)
- Using a stat visual: [wz2100/general/artifacts.json](/data/base/guidetopics/wz2100/general/artifacts.json)
- Limiting display to campaign mode: [wz2100/general/backstory.json](/data/base/guidetopics/wz2100/general/backstory.json)
- Multiple visuals: [wz2100/general/commandpanel.json](/data/base/guidetopics/wz2100/general/commandpanel.json), [wz2100/units/sensors/_index.json](/data/base/guidetopics/wz2100/units/sensors/_index.json)

# Displaying Guide Topics

Guide topics can be added to the list of available / visible topics via the `addGuideTopic` JS function.

By default, all guide topics are made available in skirmish / multiplayer modes.

In campaign mode, it's expected that the campaign scripts will gradually add guide topics as pertinent events occur.

## addGuideTopic(guideTopicID[, showFlags[, excludedTopicIDs]])

Add a guide topic to the in-game guide.

guideTopicID is expected to be a "::"-delimited guide topic id (which corresponds to the .json file containing the guide topic information).
> For example, ```"wz2100::structures::hq"``` will attempt to load ```"guidetopics/wz2100/structures/hq.json"```, the guide topic file about the hq / command center.

guideTopicID also has limited support for trailing wildcards. For example:
- ```"wz2100::units::*"``` will load all guide topic .json files in the folder ```"guidetopics/wz2100/units/"``` (but not any subfolders)
- ```"wz2100::**"``` will load all guide topic .json files within the folder ```"guidetopics/wz2100/"``` **and** all subfolders

(The wildcard is only supported in the last position.)

showFlags can be used to configure automatic display of the guide, and can be set to one or more of:
- ```SHOWTOPIC_FIRSTADD```: open guide only if this topic is newly-added (this playthrough) - if topic was already added, the guide won't be automatically displayed
- ```SHOWTOPIC_NEVERVIEWED```: open guide only if this topic has never been viewed by the player before (in any playthrough)

You can also specify multiple flags (ex. ```SHOWTOPIC_FIRSTADD | SHOWTOPIC_NEVERVIEWED```).

The default behavior (where showFlags is omitted) merely adds the topic to the guide, but does not automatically display / open the guide.

excludedTopicIDs can be a string or a list of string guide topic IDs (non-wildcard) to be excluded, when supplying a wildcard guideTopicID.
