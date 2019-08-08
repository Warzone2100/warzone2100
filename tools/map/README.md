# Warzone map tool

Tool for viewing and generating previews of Warzone2100 maps.
Works only with old map format generated with FlaME.

- [x] Read old formats of Warzone2100 maps.
- [x] Read FlaME comments in file headers.
- [x] Make preview images.
- [x] Tweak colors of cliffs, terrain, water tiles.
- [x] Analyze map by structures count.
- [x] Print usefull info to console.
- [x] Provide easy wrapper to Warzone map format.
- [x] Windows support. (see below)
- [ ] Read new map format. (json) (will be added soon)
- [ ] Create map renders. (like from ingame)
- [ ] Edit maps. (Will be added in a sperate app)
- [ ] GUI for Windows users.

# How to build

1. Install deps.
 That can be done with `$ sudo apt-get install g++ libstdc++-6-dev lib32gcc-7-dev libc6-dev make git`
2. Clone this repo `$ git clone https://github.com/maxsupermanhd/WMT.git`
3. Go to directory `$ cd WMT`
4. Make `$ make`

# How to use

1. Find some Warzone map
2. Run WMT with supplied map file (includes path)
3. Open with any png viewer/editor generated image.
4. For more info/tweaks see `--help` argument

# How to run on Windows based OS

Just use .exe file from releases. (use cmd to run)

# Issues recommended template

- OS (Linux/Windows) (include kernel/libs versions).
- Type: crash(segfault/abort), infinitie loop, opening error or saving error. (or write something other)
- Logfile generated with piping WMT output with -v999 to file.
- Map file to reproduce error.

*Warning!*
*I will not respond to any windows-only issues.*
*I don't have any windows PC. If you want help with your problem on Windows based system you should ask someone else.*

*Explanation*
*Any zip file that WMT opens stores in dynamicly allocated memory and opens it with `fmemopen` function.*
*In Windows there is no true alternative for this kind of operations, so I decided to make temp-files crutch solution to Windows builds.*
*If you want to help making Windows releases please contact me.*

# Contact me

You can write me in VK https://vk.com/1dontknow2 or mail q3.max.2011@ya.ru
PR welcome. Open issues if you have any problems.

