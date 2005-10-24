========================================================
Warzone 2100 GPL
Version 2.0.2.3 (beta)
========================================================

********
NOTE, in game, during single player games, the user must hit the ESC key
to continue the briefing screens.  This is done so that you may see what
to do next...
This is a temporary solution.
For more details, please read the Video section below.
********

========================================================
========================================================

Welcome back !

Warzone 2100 was an innovative 3D real-time strategy game back in 1999, and its
source code was released on December 6th, 2004, under the GPL (see COPYING.txt
in this directory for details). Soon after that, the Warzone 2100 GPL project
was formed to take care of its future.

Our goal is to make warzone run on all possible platforms. Therefore, we
stripped the original code of all proprietary technologies and replaced them
with cross-platform and free equivalents, like OpenGL, OpenAL, etc... Right
now, Warzone 2100 works very well on Windows and GNU/Linux, and it should be
portable to most platforms where the required libraries are available with
little effort (but right now the code is limited to 32 bit, little-endian
platforms).

Unlike Pumpkin Studios, the current developers aren't working on
Warzone 2100 GPL professionally, and if we don't meet your expectations,
be patient or join us if you think you can help us.

We are in need of coders and testers for all platforms (Windows, Linux,
Mac...). We are also looking for graphics artists and modellers to remake the
models and textures.

Our project is located here:
http://developer.berlios.de/projects/warzone/

Warzone fan site forums are here:
http://www.realtimestrategies.net/forums/index.php
You'll have most chances to find us in the development section.

Other Warzone 2100 Fan sites are available @
Warzone 2100 Link Turret : http://wzlinkturret.1go.dk/


Also PLEASE report all bugs you may find to either:

bugtracker @ http://developer.berlios.de/bugs/?group_id=2909

or if you dislike that, then post in the forums @
http://www.realtimestrategies.net/forums/index.php
or send it to our mailing list @
warzone-dev@lists.berlios.de

Be as detailed as possible, and note the revision number of the
game.

Thanks!


--------------------------------------------------------------------------------
1) Command-line options.

Note, command line options apply to BOTH windows & linux users.

For windows users, you can make a shortcut and if the game is install in
"c:\Warzone 2100 GPL", the shortcut command line would be
"c:\Warzone 2100 GPL\warzone.exe -window -800x600".

You can also run it by opening a command window, changing to the correct
directory ("c:\Warzone 2100 GPL" in this case) and typing
"warzone -window -800x600".

For linux users:
Once the game is installed (see INSTALL.txt in this directory), you can run it
by changing to its directory and typing './warzone'. Here's a few of the most
common command line options :
	-fullscreen : runs in a full-screen window
	-window : runs in a window
	-WIDTHxHEIGHT : runs at WIDTH times HEIGHT resolution, replace WIDTH
		and HEIGHT with your dimensions of choice. Beware, though,
		as it needs to be a resolution your X server can display.

Note that you do need to type the dash in front of the option, like this :
		./warzone -fullscreen -1280x960

The fullscreen/window modes and the resolution are stored, so they only need to
be specified once, or when you want to change them. So,
		./warzone
will start in fullscreen and in 1280x960 because this is what I specified just
before (remember ?).

For more groovy command-line options, check src/clparse.c (yes, we promise we'll
document this properly some day).


2) Configuration

For windows users:
This game uses the registry for most settings.  So no need to worry about a
config file.

For linux users:
When you start Warzone 2100 for the first time, a '.warzone2100' directory
is created in your home directory. A certain number of files are also created in
it, the most important being the one called 'config'. In case you are upgrading
from a previous version and experience problems, try and remove this
'~/.warzone2100/config' file.

3) Videos

We do NOT have permission to distribute the original game videos.
The videos were made using a custom RPL codec by Eidos, which is also used
in most of their other early games, like Tomb Raider.  There is no codec
as far as we know for linux people.  There is one for windows, but at this
time, we do not play them either for windows, even if you have the videos
on your HD.  You will hear the sound from the videos if you have the videos
installed.


4) Music

We are not able to distribute the original music, but we have included two free
tracks. If you don't like them, you can use a custom playlist.

The following playlists are read, in this order, and only the first playlist
found is used :
- ~/.warzone2100/music/music.wpl
- <whereverwarzoneisinstalled>/music/music.wpl

Playlist example :

--------------------------------------------------------------------------------
[game]
path=.
shuffle=yes
neos_aurore.ogg
neos_chocolat.ogg
neos_down.ogg
neos_esperance.ogg
neos_indy.ogg

[menu]
path=.
neos_symphonie_du_vide.ogg
--------------------------------------------------------------------------------

In this example, there are 5 songs played in random order while playing a game,
and one song only when going around in the menu.

"shuffle=yes" specifies you want the songs for a given category (game or menu)
to be played in random order. If you want it for both categories, you need to
specify it in both of them.

"path" can be anything you wish. If it's ".", the path is the directory where
the playlist is found. If no path is set, all filenames must be given with
absolute path.

ogg and mp3 files are supported (provided all necessary libs are installed).

---------------------------------------------------------------------------------

Special thanks to RBL-4NiK8r, he has given us permission to distribute the
following maps with the game: 4c-borgwars1.wz, 4c-craterlake.wz, 4c-scav3.wz,
4c-theweb.wz, 4c-zmazetest.wz, 8c-iamrage.wz,8c-nightmaredone.wz, and
8c-teamwar2d.wz.


                                               The Warzone 2100 GPL Development Team.


