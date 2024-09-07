# Briefing and Proximity file formats

In releases beyond the 4.5 series, the campaign brief and Proximity files are now in a more readable format.

# Proximity

Example:
```json
{
    "C1A_BASE0": {
        "audio": "pcv390.ogg",
        "message": "BARBASE_MSG",
        "type": 0,
        "x": 3904,
        "y": 4672,
        "z": 0
    }
}
```

Each Proximity message must have a unique ID. This ID will be what can be referenced in scripts. Various key-value pairs in one contain:
- audio: The audio file associated with this Proximity blip when it gets clicked. Either a string to an audio file, or 0 to not use a sound.
- message: A translated string reference that will print a message when the Proximity blip is clicked on. An array of references is supported.
- type: What type of Proximity blip this is (0 - ENEMY|RED, 1 - RESOURCE|BLUE, 2 - ARTIFACT|GREEN).
- x, y, z: Coordinates in World Units (128 World Units = 1 Tile) where the Proximity blip will appear.
