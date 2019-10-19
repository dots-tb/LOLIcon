# LOLIcon 
## (LOLIconsole Offends Little Idiots Console)

by @dots_tb

A  small onscreen console that runs from the kernel level. So it probably won't crash games as many certain user plugins do. It also removes dependency of kuio.

(Also some advice FOR DEVS, use ioPlus which allows you to use normal sceIo functions in user plugins <https://github.com/CelesteBlue-dev/PSVita-RE-tools/tree/master/ioPlus/ioPlus-0.1>).

Oh yeah, and it adds 500mhz (aka real overclocking). Enjoy...The kernel level allows for more options in regards to overclocking. 

 ### (The menu may lag, but it will go fullspeed when you exit). 

### WARNING: THIS IS OBVIOUSLY EXPERIMENTAL AND CARRIES THE DANGERS OF OVERCLOCKING (FOR REAL OVERCLOCKING). Please proceed with caution.

### Further note: Yifan Lu has labeled this mode as 494mhz, but seeing how the rest of the pervasive values correspond to a value slightly off, Celeste and I have decided to label it 500mhz instead.

### PS: Userland apps will report false speeds, this is LIBEL and FAKE NEWS. We have provided an indicator for the direct registers that control the ARM clock speeds on the overclock page to compare to the wikipage located below.

Other features: Quickly exit game, maintains configuration for each process(including shell)*, FPS counter, x and o button swap, allows for proper default game clocking, you can also overclock shell's boot up. There might be more... and more to come.

*Suspending to shell and resuming game will LOAD A CONFIGURATION (DEFAULT OR SAVED). You will lose your changes when changing a task (including suspending to shell) unless you save.

Video courtesy of @Yoyogames28 :

https://www.youtube.com/watch?v=mNPscIVubOY

Video courtesy of castelo:

https://www.youtube.com/watch?v=ATiv301_eOA


Installation
--------------------------------------------------------------------------------

Put "lolicon.skprx" in 'tai' folder in the root of your Vita. (Or just use autoplugin, because I'm sure a ver that supports this is out already)

In the 'tai' folder, add the filepath to the "config.txt" file, under the 'KERNAL' section.
```text
*KERNEL
ur0:tai/lolicon.skprx
```

After that just reboot and press SELECT + UP to enable menu. Press SELECT + DOWN to close menu.

Thanks to: 

https://wiki.henkaku.xyz/vita/Pervasive#ARM_Clocks

https://github.com/frangarcj/oclockvita

https://github.com/Scorpeg

https://github.com/BeatPlay/BetterAmphetaminPlugin

https://github.com/DrakonPL/VitaJelly

https://github.com/Rinnegatamante/Framecounter/blob/master/main.c

https://github.com/joel16/PSV-VSH-Menu/tree/master/source

## Major Thanks:

@CelesteBlue123 - Help. TREMENDOUS and loyal DEV. Only the BEST! I recommend you retain the services of this man. HE DOES NOT CRACK!

@nyassen (sys) - some guy

@SilicaVita - this mysterious person helped with the name, not afraid to keep spamming that he did.

## PR and special testing:

@Yoyogames28 - Discount DF and cute boi

castelo - Great tester, thanks to @Cimmerian_Iter for letters of rec. 

@froid_san 3.68 tester... I forgot to test on 3.68. Sad!

## Testing team:

@juliosueiras

GrandMaster

@nkekev

@coburn64

Orphen07

CrossFusionX
