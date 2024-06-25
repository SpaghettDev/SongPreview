# SongPreview

This mod allows you to play a snippet ranging from 5 to 20 seconds (changed in the mod's settings) when clicking the preview button on a song widget.

Features:
- Show preview button on all song widgets (even the ones in the level selection screen)
- Preview both downloaded and non-downloaded songs
- idk bruh

The snippet is fetched and downloaded on button click, then it is cached until the next game launch, where it is deleted.
If the song isn't able to be downloaded, or FMOD (the game's audio engine) isn't able to play the snippet, the preview button will become gray and unlickable.

