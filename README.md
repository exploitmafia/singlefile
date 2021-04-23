# singlefile

This is a featured CS:GO internal cheat written in less than 1000 lines, and in one C++ file. I encourage you to submit feature suggestions and issues with the cheat!

## features
* draggable gui

![gui image](img/img1.png)

thanks es3n1n for making the menu draggable!

* config system
* bhop
* auto pistol
* hit sound
* box esp

![esp image](img/img2.png)

* name esp
* health bar
* dormant & team check for esp
* spectator list
* disable post processing
* noscope crosshair
* recoil crosshair
* auto accept
* triggerbot
* radar
* rankrevealer
* use spam
* flash reducer

## config
a simple config system is included with this cheat. there is currently only one config, however we plan to expand this.. the save & load buttons operate on a config file "Counter-Strike: Global Offensive\singlefile.cfg", where the config is stored as a binary representation of the config structure. Unfortunately, a more user editable config system such as JSON is not possible due to size constraints.
## code style
this was programmed with the intention of being a single-file cheat, as well as a target line count of under 1000. i achieved both of these with the initial version i am posting.

## features i won't add
in the interest of keeping line count down, i won't be adding customizable chams or glow, as the colors seem too awkward to emplace in the menu. Right now triggerbot is controlled by a macro, something I don't really like but currently works. I will be adding a key binder in the menu, which will make triggerbot easier. A keyboard is pretty simple compared to a colorpicker, so something I can reasonably add. As well as this, glow and chams are a rabbit hole of interfaces and classes that will just have like 1 or 2 functions. Glow doesn't seem that bad, however you need to have a visibility check. 

## changelog
### v1.0
adds esp, as well as basic misc.
## v1.1
adds triggerbot, radar, and other misc functions
## v1.2
adds draggable menu, rank revealer, use spam and flash reducer
### v1.2.1
minor bug fixes, increment cs:go version
