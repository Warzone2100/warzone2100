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

# Briefing

Example:
```json
{
    "video0000": {
        "name": "MB1A_MSG",
        "sequences": [
            { "audio": "", "loop": 1, "subtitles": "TRANS_MSG1", "video": "brfcom.ogg" },
            { "audio": "", "loop": 0, "subtitles": ["CAM1A_MSG1", "CAM1A_MSG2", "CAM1A_MSG3"], "video": "cam1/cam1ascv.ogg" }
        ]
    }
}
```

Each video sequence should start with something simple like "video..." or something else at your discretion. Order here does not matter.
Within each sequence, there will be two values:
- name: The unique view data name ID. This will be used as a reference for scripts to invoke to start a video subset.
- sequences: an array of objects (order matters!) containing variables about each sub-video:
  - audio: An audio file string that will play before the video starts.
  - loop: An integer between 0-1 to loop the entire video until its audio stops playing. Will always display subtitles every frame if set to 1.
  - subtitles: Either a string or array of string translation references that will be used in this video.
  - video: The actual video file to display. Note the directory starts at "data/[base|mp]/sequences/".
