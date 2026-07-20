# Add your own music to Warzone 2100

## 1. Find the music folder
1.  Start Warzone 2100
2.  Click **Options**
3.  Click **Open Configuration Directory** in the bottom left corner
4.  Open the **music** folder

## 2. Create an album to hold your music

1.  If it does not exist yet, create a folder called **albums**
2.  Inside, create another folder called **My Album**
3.  Add your song(s) in `.ogg` or `.opus` format
4.  Add an album cover image and name it `albumcover.png`
5.  Add a file named `album.json`

Your folder structure should look like:
```
music/
└── albums/
    └── My Album/
        ├── album.json
        ├── albumcover.png
        ├── Song1.opus
        └── Song2.ogg
```
## 3. Configure the album

Paste the following into `album.json`:
```
{
  "title": "My Songs",
  "author": "Various Artists",
  "date": "Various Dates",
  "description": "Songs I added",
  "album_cover_filename": "albumcover.png",
  "tracks": [
    {
      "filename": "Song1.ogg",
      "title": "Song 1 Title",
      "author": "Song 1 Author",
      "default_music_modes": ["campaign", "challenge", "skirmish", "multiplayer"],
      "base_volume": 100,
      "bpm": 121
    },
    {
      "filename": "Song2.opus",
      "title": "Song 2 Title",
      "author": "Song 2 Author",
      "default_music_modes": ["campaign", "challenge", "skirmish", "multiplayer"],
      "base_volume": 100,
      "bpm": 135
    }
  ]
}
```
Modify the fields or add more tracks, as needed.

## 4. Play your music!

1.  Restart Warzone 2100
2.  Wait for your song(s) to begin playing, or play the song(s) manually via the Music Manager.
