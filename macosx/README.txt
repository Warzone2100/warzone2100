This directory contains support files for the MacOS X port of Warzone 2100.
Run the script 'makedmg.sh' to create 'Warzone 2100.dmg', containing a
fully-self-contained application bundle to run Warzone 2100 on MacOS X.

Building Warzone 2100 on MacOS X is currently only known to work on
MacOS X 10.4 "Tiger" with DarwinPorts and the following ports installed:
  jpeg
  libmad
  libogg
  libpng
  libsdl
  libsdl_net
  libvorbis
  physfs

DarwinPorts puts files in /opt/local, so you will have to run the configure
script as follows (line-ending backslashes continue the command on the
following line):

CFLAGS=-I/opt/local/include \
CPPFLAGS=-I/opt/local/include \
LIBS=-L/opt/local/lib \
./configure

After compiling the software by running 'make' in the toplevel directory,
you can run makedmg.sh to create the final disk image file.

When you run the program, it will create a directory named .warzone-2.0
in your home directory.  You can control settings such as the screen
resolution in the 'config' file within that directory.  The setting to
change the screen resolution is 'resolution=<setting>'.

