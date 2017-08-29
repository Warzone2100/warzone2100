Splits and scales tertile texpages into mipmaps.

version "1.2" author "Kevin Gillette"

Finds and filters files based on the 'extension' configuration setting, and
of those, groups where available to allow mipmap output to be created from
as few as one input texpage and as many texpages as there are output
resolutions.

Filenames are grouped together if, after all cased letters are lowered and
file extensions are stripped, they have identical names. if, after the
dotted is stripped they still have at least one period in the filename, and
only numeric digits proceed that period, then all text up to, but not
including the last remaining period is used in the comparison. Thus, the
following filenames are grouped together to represent different resolutions
of the same conceptual texpage:

tertilec1hw.tga
tertilec1hw.53.pcx
tertilec1hw.128.pcx

However, 'tertilec1hw.a23.pcx' will create a new 'tertilec1hw.a23' group.

Files are handled on a first-come basis, if two files that are considered
to be part of the same group also have the same resolution, the first one
found will be used, and all subsequent ones with the same resolution in
that group will be discarded. Since this behavior is unpredictable and
operating-system-dependent, it should not be relied upon, and the user
should take care to have have no more than one texpage of each resolution
per group.
=======================================================================================
To have a fully working Windows version of the texpage2mipmap.py  tool  please follow the instructions below.


installing python  - get the windows installer from here - http://www.python.org/download/   (Windows binary -- does not include source)

-------------------------------------------------------------------------------------

installing imagemagik  - Windows Binary Release

ImageMagick runs on all recent Windows releases except Windows 95. We recommend its use on an NT-based version of Windows (NT4, 2000, 2003, XP, or Vista). Starting with ImageMagick 5.5.7, older versions such as Windows 95 are not supported anymore. The amount of memory can be an important factor, especially if you intend to work on large images. A minimum of 256 MB of RAM is recommended, but the more RAM the better.

The Windows version of ImageMagick is self-installing. Simply click on the appropriate version below and it will launch itself and ask you a few installation questions. Versions with Q8 in the name are 8 bits-per-pixel component, whereas, Q16 in the filename are 16 bits-per-pixel component. A Q16 version permits you to read or write 16-bit images without losing precision but requires twice as much resources as the Q8 version. Versions with dll in the filename include ImageMagick libraries as dynamic link libraries. If you are not sure which version is appropriate, choose http://www.imagemagick.org/download/binaries/ImageMagick-6.3.5-6-Q16-windows-dll.exe

To verify ImageMagick is working properly, type the following in an MS-DOS Command Prompt window:

  convert logo: logo.gif
  identify logo.gif
  imdisplay logo.gif

Congratulations, you have a working ImageMagick distribution under Windows and you are ready to use ImageMagick to convert, compose, or edit your images or perhaps you'll want to use one of the Application Program Interfaces for C, C++, Perl, and others.

-------------------------------------------------------------------------------------

texpage2mipmap.conf     

## = comments   if no ##  the line will be processed with whatever is in it.

## this file must have the same pre-extension name as the script, plus .conf
## so if the script is 'texpage-convert.py', this file will not get read unless
## it is named 'texpage-convert.conf'

## for any directive expecting a file path or name as its value: whitespace
## (excluding end-of-lines) does matter, so leading or trailing spaces will
## be taken literally. any line beginning with a '#' character will be ignored.
## for everything else, the format is as follows: 'directive-name=value', with
## absolutely no spaces where not needed (if you use whitespace liberally, you
## might get lucky, but you won't be shown any mercy). any directive omitted or
## left commented will cause this script to use sensible defaults. in very few
## cases, leaving a directive's value blank will cause coded defaults to be
## used

## the path that the imagemagick commandline utilities reside in. leave it
## blank or commented for autodetection via the PATH variable. set this only
## if you are having problems.

#impath=

## a set of resolution-specific directories will be created here for each valid
## input file. if your mod's files are in "c:\mymod" or "/home/me/mymod/" it
## would be sensible to set this to "c:\mymod\texpages" or
## "/home/me/mymod/texpages/", respectively, which would allow for quick
## packaging of a wz archive. ending the pathname with a / or \ is optional but
## not required. both relative and absolute pathnames are supported. defaults
## to '.' (same directory as this script).

#export-path=

## a file in the current directory with one of these endings will be processed
## defaults to '.png .pcx .tga' if not specified. order is not important.

#extensions=.tga

## these are the tile-resolutions to output. an input file will be ignored if
## the tile-resolution is not listed here. warzone only supports power-of-two
## resolutions. defaults to '16 32 64 128'. make sure that *only* numerical
## digits and spaces exist after the equal-sign, or the script will abort.

#resolutions=16 32 64 128

## these choose the imagemagick scaling filter to use whenever the tile
## resolution is increased or decreased. a value of 'default' will cause the
## imagemagick default to be used. filter-increase uses the 'Point' filter by
## default, and filter-decrease uses the imagemagick default unless otherwise
## specified. available filters can be found at:
## http://www.imagemagick.org/script/command-line-options.php#filter

filter-increase=Point
#filter-decrease=

## if specified, all script output to stdout after this file has been parsed
## will be redirected to the specified filename. notably useful for windows
## users encountering problems

log=log.txt

## not to be used lightly, if changed from its default of 9, correct warzone
## texpages will either not load or will load incorrectly. if you are creating
## new tertile pages, then stick with the standard, and make the image exactly
## wide enough to hold 9 tiles. must be an integer or the script will abort.

#columns=9

===================================================================================

filter types to use with texpage2mipmap.py
 
-filter type
use this type of filter when resizing an image.

Use this option to affect the resizing operation of an image (see -resize). Choose from these filters:

   Point
   Box
   Triangle
   Hermite
   Hanning
   Hamming
   Blackman
   Gaussian
   Quadratic
   Cubic
   Catrom
   Mitchell
   Lanczos
   Bessel
   Sinc
The default filter is automatically selected to provide the best quality while consuming a reasonable amount of time. The Mitchell filter is used if the image supports a palette, supports a matte channel, or is being enlarged, otherwise the Lanczos filter is used.

Since options are evaluated in command line order, be sure to specify the -filter option before the -resize option. 

Use the -list option with a 'Filter' argument for a list of -filter arguments available in your IM installation.

