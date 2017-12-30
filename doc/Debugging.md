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
to run your own javascript code in the selected script context while
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

Cheats
------

There are some cheat codes that might help. For example, if you are
tracking down an orders related bug, then 'showorders' will show
you shorthand strings on each unit about which order it currently
has.

One very useful cheat is 'research all', which teaches you (and
your allies, if shared) every available research in the game.

The 'let me win' cheat allows you to skip to the next campaign
level.
