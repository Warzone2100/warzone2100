Debugging issues
================

Here are some hints and tips for how to debug Warzone problems.

In-game tools: Debug menu
-------------------------

The first tool you should learn how to use is the debug menu. You can
open it by going into cheat mode (shift+backspace by default), then
press ctrl+o. The 'Selected' tab can give you information about the
currently selected game object - if you are missing some information
here, please add it and submit a patch. This tab does not update by
itself, so click again to refresh.

The 'Contexts' tab shows you the script contexts currently active,
and their global variables. The 'Run' button in this tab allows you
to run your own JavaScript code in the selected script context while
the game is running.

In-game tools: Object tracing
-----------------------------

When in debug mode, you can press ctrl+l to trace one game object.
This object may now start to emit a lot of information on standard
out. If you are missing some information, just go into the source
and add objTrace(id, ...) calls. This is especially useful when
adding a debug() or printf() call would cause way too much spam on
the standard out. Please think twice about committing objTrace()
calls that could be called every frame, as this makes them less
useful for others. Other than that, adding more trace calls is a
good idea, as they are very low overhead when not active.

Anything includes the campaign library has access to camTrace(...).
These can be placed in scripts to show the value of something or something like
printing a string to the terminal when in cheat mode.

Cheats
------

There are some cheat codes that might help. For example, if you are
tracking down an orders related bug, then 'showorders' will show
you shorthand strings on each unit about which order it currently
has.

One very useful cheat is 'research all', which teaches you (and
your allies, if shared) every available research in the game.

Another useful cheat is 'deity'. It allows you to see everything on the map
much like having an uplink built. It does cause radar blips to misbehave if
used repeatedly in campaign.

'biffer baker' makes everything you own very tough to destroy.

The 'let me win' cheat allows you to skip to the next campaign
level.

There is a cheat exclusive to campaign with the format 'ascend sub-X-Ys'. Do
note that research is not granted from the skipped missions.
X = what campaign you are on (Alpha = 1, Beta = 2, Gamma = 3).
Y = mission number.
This cheat only warps to pre-offworld missions and not those where a mission
is active on map, ie Alpha 2. So if for example you want to skip to
the last Alpha offworld campaign mission type 'ascend sub1-ds'.

Possible levels to warp to:
1-1s, 1-2s, 1-3s, 1-4as, 1-5s, 1-7s, 1-ds,
2-1s, 2-2s, 2-5s, 2-ds, 2-6s, 2-7s, 2-8s,
3-1s, 3-2s, 3-4s.

Used in conjunction with 'let me win' you will be able to get to any mission
in the campaign fairly quick.
