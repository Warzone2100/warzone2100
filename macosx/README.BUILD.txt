This directory contains support files for the MacOS X port of Warzone 2100.
Run the script 'makedmg.sh' to create 'Warzone 2100.dmg', containing a
fully-self-contained application bundle to run Warzone 2100 on MacOS X.
This application bundle should include all the libraries and frameworks
necessary to run the game on any MacOS X 10.3 "Panther" or MacOS X 10.4
"Tiger" system without installing anything.  To just make the application
bundle without creating a disk image, run 'makebundle.sh'.

Building Warzone 2100 on MacOS X is currently only known to work on
MacOS X 10.4 "Tiger" with DarwinPorts and the following ports installed:
  automake
  jpeg
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
LDFLAGS=-L/opt/local/lib \
./configure

You may be able to build Warzone 2100 with the necessary libraries
installed in another location.  This has not yet been tested.  Building
for the Intel architecture has also not yet been tested.

The script 'consolidatelibs.rb' attempts to find all non-system libraries
and frameworks that the executable requires and copy them into the
application bundle, updating dynamic library links accordingly.

After compiling the software by running 'make' in the toplevel directory,
you can run makedmg.sh to create the final disk image file.

See the file README.txt (which is also included in the disk image that
makedmg.sh generates) for more information on configuring and running
the program.

