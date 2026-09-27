# Sequence Subtitles JSON

Text shown over a video (subtitles, titles, captions) can be placed in areas of the video with a JSON
subtitle file. The engine lays the text out: it wraps and centers each line within its area, and sizes the
text to fit. Text subtitle files (`.txt`) work as before.

- [Files](#files)
- [Areas](#areas)
  - [Area keys](#area-keys)
- [Subtitle lines](#subtitle-lines)
  - [Line keys](#line-keys)
- [Layout](#layout)
- [Translatable strings](#translatable-strings)
- [Example JSON](#example-json)

# Files

Subtitle files are located underneath the `sequenceaudio/` folder in WZ's virtual filesystem, like text
subtitle files.

- A video's subtitle file is `sequenceaudio/<video>.json`: the video's path (under `sequences/`) with its
  extension replaced by `.json`. If there is no such file, the text file `sequenceaudio/<video>.txt` is used.
- A briefing sequence can name its subtitle file with `"textFile"` (see
  [BriefAndProximityFormat.md](BriefAndProximityFormat.md)), a `.json` or a `.txt` file. Several briefings can
  then share one video while each shows its own text.
- Areas that several subtitle files share are defined in an areas file, which each of them names with
  `"areasFile"`. A subtitle file can add areas or change shared ones.
- A JSON subtitle file is loaded whatever the subtitles setting: each area decides whether its text shows when
  subtitles are off (`"alwaysShow"`). Text subtitle files only load with subtitles on.

# Areas

An area is a rectangle of the video that text is placed in. Areas are defined in an areas file, for example
`sequenceaudio/mymod_areas.json`:

```json
{
    "type": "wz2100.subtitleareas.v1",
    "reference": [640, 480],
    "areas": {
        "header": { "x": 23, "y": 23, "width": 454, "height": 74, "padding": 4, "fontSize": 9, "valign": "center" },
        "title": { "x": 23, "y": 200, "width": 594, "height": 40, "fontSize": 11, "alwaysShow": true }
    }
}
```

- `"reference"`: `[width, height]` of the video in its own pixels. Positions and sizes of areas are in these
  units, and are mapped onto wherever the video is shown, for any screen size and video mode.
- `"areas"`: an object of named areas.

A subtitle file uses the areas of the file its `"areasFile"` names, a path under `sequenceaudio/`. Give an
areas file a name of its own (for example, after the mod), so that it can't replace another mod's or the
game's areas file.

A subtitle file can also define areas of its own, in an `"areas"` object:

- An area with a new name is added to the areas from the areas file.
- An area with the same name as one in the areas file changes only the keys it gives. For example,
  `"header": { "fontSize": 11 }` makes the text of the areas file's `header` bigger, and keeps its position,
  size and other keys.
- Positions and sizes in the subtitle file are in the units of its own `"reference"`. If it has none, the
  areas file's `"reference"` is used.

## Area keys

- `x`, `y`, `width`, `height`: the area.
- `fontSize`: the size of its text.
- `padding` (optional, default 0): space kept free inside the area's edges.
- `align` (optional): horizontal alignment of each line. Only `"center"` is supported (the default).
- `valign` (optional): vertical alignment of the text in the area, `"top"` (the default) or `"center"`.
- `overflow` (optional): `"clip"` (the default) shows only the lines that fit, and ends the last one with an
  ellipsis. `"overflow"` shows every line, past the bottom of the area if needed.
- `alwaysShow` (optional, default `false`): shows the area's text even with subtitles turned off. Use it for
  text that is part of the video - for example: titles, or the captions of a video without speech.

# Subtitle lines

A subtitle file lists the lines to show:

```json
{
    "type": "wz2100.subtitles.v1",
    "areasFile": "mymod_areas.json",
    "lines": [
        { "start": 11.0, "end": 13.0, "area": "header", "textId": "L1_BMSG1" },
        { "start": 13.0, "end": 17.0, "area": "header", "speaker": "CLAYDE", "textId": "L1_BMSG2" }
    ]
}
```

## Line keys

- `start`, `end`: when the line shows, in seconds of video time.
- `area`: the name of the area it shows in.
- Either `text`: the text to show. \
  Or `textId`: the ID of a string from the loaded string tables (`messages/strings/`), for example a briefing
  message, which shows that string. A missing ID shows the ID itself (and is reported as an error).
- `speaker` (optional): a name shown before the text, as `<SPEAKER>: `. Leave it out for a string that already
  starts with its speaker.

# Layout

- An area shows one line at a time. A line's text wraps onto as many rows as it needs. If two lines of an area
  overlap in time, the one that started later is shown (and the overlap is reported as an error).
- An area uses one text size for all of its lines in a file: its `fontSize`, made smaller if needed until every
  one of its lines fits, but no smaller than the smallest size the engine uses for text (on a small video, text
  is shown at that size even if `fontSize` maps to less). Text that still doesn't fit is clipped or overflows
  (`"overflow"`).
- The text is white with a grey outline, as text subtitles are.

# Translatable strings

- `text` values and `speaker` names are translatable (speaker names with the context `speaker`).
- A `textId` shows its string as translated where the string is defined.

# Example JSON

`sequenceaudio/act1/A1L1intro.json`, the titles over an intro video:

```json
{
    "type": "wz2100.subtitles.v1",
    "areasFile": "mymod_areas.json",
    "areas": {
        "titleName": { "x": 23, "y": 240, "width": 594, "height": 24, "fontSize": 11, "alwaysShow": true }
    },
    "lines": [
        { "start": 1.0, "end": 5.0, "area": "title", "text": "ACT 1" },
        { "start": 2.5, "end": 5.0, "area": "titleName", "text": "WELCOME TO THE JUNGLE" }
    ]
}
```
