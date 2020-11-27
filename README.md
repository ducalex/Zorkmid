# About

Zorkmid is a Z-Vision engine written in portable C99. It allows you to play Zork Grand Inquisitor and Zork Nemesis.

It is based on [ZEngine by Marisa-Chan](https://github.com/Marisa-Chan/Zengine).

# Screenshots
![menu](https://raw.githubusercontent.com/ducalex/Zorkmid/master/res/screenshot1.png)
### Widescreen
![village](https://raw.githubusercontent.com/ducalex/Zorkmid/master/res/screenshot2.png)

# Compilation

### Prerequisites
- GCC or MinGW
- SDL, SDL_mixer, SDL_ttf
- SDL_gfx

### Build
- `make win32` or `make linux`


# TODO
````
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
````

# License

MIT License