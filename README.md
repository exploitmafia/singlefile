If you have problems injecting, read below!
# singlefile

This is a featured CS:GO internal cheat written in less than 1000 lines, and in one C++ file. I encourage you to submit feature suggestions and issues with the cheat!

If anyone who has contributed in the past and had received a push to main from their pull request, I'd be happy to give you access to the beta branch if you want to contribute further without making a pull request.

You can join [here](https://github.com/exploitmafia/singlefile/invitations)

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
* in game triggerbot
* radar
* rankrevealer
* use spam
* flash reducer
* vote revealer

## common issues
If you're having problems injecting, I encourage use to use SingleFile Injector, an injector that bypasses's CS:GO LoadLibrary check. You can find it in the SFI folder.

## config
a simple config system is included with this cheat. there is currently only one config, however we plan to expand this.. the save & load buttons operate on a config file "Counter-Strike: Global Offensive\singlefile.cfg", where the config is stored as a binary representation of the config structure. Unfortunately, a more user editable config system such as JSON is not possible due to size constraints.
## code style
this was programmed with the intention of being a single-file cheat, as well as a target line count of under 1000. i achieved both of these with the initial version i am posting.

## changelog
### v1.0
adds esp, as well as basic misc.
## v1.1
adds triggerbot, radar, and other misc functions
## v1.2
adds draggable menu, rank revealer, use spam and flash reducer
### v1.2.1
fixes sound issue
## v1.3
adds customizable triggerbot key, vote revealer and code cleanup
