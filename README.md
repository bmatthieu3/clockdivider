# NablaSynth - Clock Divider module

See it in action [here](https://youtu.be/5VTIWuq25X8)! 

A clock divider is usually a very important module in a modular synth as it can allow you to give the tempo to many other modules of your synth.
Here are some useful examples showing why it can be good to have a clock divider:

* Provide a /2, /4, /8 clock allowing to trigger drum sounds every 2, 4 or 8 cycles or any other events.
* Do Polyrhythm thanks to impair dividing ratio: /3, /5, /7!
* Mess a little bit the rythm thanks to a CV rotating input. This input accepts 0->5 volts. 0 volt means the outputs are not rotated, the first output gives a clock having the same frequency, the second one having half the input clock frequency, the third having one-third of the frequency... 5/8 volt will rotate the outputs of one output: the first output will be one-eight of the input clock frequency, the second output will give the same frequency, the third output will give the half of the input frequency... the eight's output will give one-seven of the first frequency.

# What does this repo contains

* The code for the Atmega328p in clock_divider.ino
* The electrical scheme for the module openable with kicad
![scheme](https://github.com/bmatthieu3/clockdivider/blob/master/images/scheme.png)
* The pcd footprint for kicad (There is a 1k resistor missing between VCC (5V) and pin 6 of the uC. This is for the dividing potar to work, the main features (division of the clock signal frequency by 1 -> 8, the reset of the outputs and the rotating CV) will work without correcting this).
* The .svg showing the interface plate of the module. This is a very simple one that I did very fast, it is not very good at all. I recommand you to do your own if you want to build the module for your own synth. It will be of your taste and what is from you is always the best as a DIYer.
* The .stl file of the interface plate (made with blender)
* The .stl transformed in .gcode to print it.

# What does it looks like
![front](https://github.com/bmatthieu3/clockdivider/blob/master/images/front%20panel.jpg)
![back](https://github.com/bmatthieu3/clockdivider/blob/master/images/back.jpg)
![side](https://github.com/bmatthieu3/clockdivider/blob/master/images/side.jpg)

