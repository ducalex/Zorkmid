# About

Zorkmid is a ZVision engine written in portable C99. It allows you to play Zork Grand Inquisitor and Zork Nemesis.

It is based on [ZEngine by Marisa-Chan](https://github.com/Marisa-Chan/Zengine).

# Misc

This engine was ported into scummvm, please use scummvm implementation because it better
https://github.com/scummvm/scummvm


[Needed lib's]
SDL
SDL_image
SDL_mixer
SDL_gfx
SDL_ttf

[Build targets:]
linux_zgi
linux_zgi_trace
linux_znem
linux_znem_trace
win32_zgi_trace
win32_zgi
win32_znem_trace
win32_znem

Targets trace - for output to stdout debug info about what scripts actions is doing now.

zgi - Zork Grand Inquisitor
znem - Zork Nemesis

win32 targets is for building by mingw, I use cross-compilation, you need to correct paths in Makefile.


cd Engine/src/

make *target*



[Useful things]
- You may change gamma level by "CTRL"+"+" and "CTRL"+"-"
- For run in fullscreen use -f key
- Switch to fullscreen ALT+ENTER
- You may run game binary file with "path/to/game/resources" parameter, but game saves will be in current dir.


[How to run it:]
For running games you need original games.

1. Copy files from disks likes as on windows (http://home.earthlink.net/~infernofilecabinet4/XpSetupsU_Z/Guest/zorkgiCD.htm).

2. Create folder "FONTS" and copy needed fonts(at least one file)

3. Create file Zork.dir (or try one from "conf" dir) and write to this file releative paths to folder with resources and to *.zfs file
   Each entry must by splited by new line
   
   Example:
   Addon
   Addon/subpatch.zfs
   ZNEMSCR/ASCR.ZFS
   ZNEMSCR/GSCR.ZFS
   ZNEMSCR/CURSOR.ZFS
   ZNEMSCR/CSCR.ZFS
   ZNEMSCR/TSCR.ZFS
   ZNEMSCR/ESCR.ZFS
   ZNEMSCR/VSCR.ZFS
   ZNEMSCR/MSCR.ZFS
   ZASSETS
   ZASSETS/ENDGAME
   ZASSETS/CONSERV
   ZASSETS/GLOBAL
   ZASSETS/GLOBAL/VENUS
   ZASSETS/GLOBAL2
   ZASSETS/MONAST
   ZASSETS/GLOBAL3
   ZASSETS/CASTLE
   ZASSETS/TEMPLE
   ZASSETS/ASYLUM
   VIDEO
   
4. For Nemesis you need to copy res/MIDI folder to game dir.

-----------------bottom--------------------

VIDEO: 
29.10.2010: http://www.youtube.com/watch?v=a88-Jgt5Grw
07.09.2011: http://www.youtube.com/watch?v=FY-epYjNAk8

Action:Region in action: http://www.youtube.com/watch?v=CuQVI2QteCM
(water ripple, blinking and clouds in the sphere)

Screenshots:
http://img850.imageshack.us/i/45844953.png/
http://img508.imageshack.us/i/38819204.png/
http://img222.imageshack.us/i/32948516.png/

Widescreeeeeeeeen so ^_________________________^:
http://img8.imageshack.us/img8/7042/47355924.png


P.S. 
Thanks w8m (for "unraw" code reverse)

------------------TODO------------------
[Actions Functions]
action_copy_file	    :reverse it (not needed ?)
action_clear_system_message :reverse it

[Other]
* Create utility and scripts for scale for dingoo
* Clear code
* Other little things...
* Add more code for pana and tilt renderers
* Add fast code for distort effect based on scale-parameter.

--------------TODO if needed (not used)-----------------

[Script Functions]
action_add_preblt	:reverse it
action_remove_preblt	:reverse it
action_set_master_volume:reverse it and write code
action_zoom		:reverse it and write code


------------------License--------------------

Mit license for source code (Bin dir not included), read file "LICENSE"