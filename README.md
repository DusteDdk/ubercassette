# UberCassette
Awesome WAV to Commodore64 TAP converter, written by Ian Gledhill. Thanks to Ian for putting the sauce online, if you find this software useful, visit his page and send a donation his way!

Official website: http://www.retroreview.com/iang/UberCassette/

# What it is for
It's for when you want to transfer your Commodore 64 tapes onto your modern PC.

After recording the tape into your computer, you should end up with a WAV file,
ubercassette can convert the WAV to the well-supported TAP format, which can
be used on emulators, and can also be converted back to WAV for use with a C64
at a later time.

# Why I put it here
Because it's an awesome program, and this way I know how to find it again later.

# How to use it
First, you need a cassette player, anything that you can connect to your computer is most likely fine.

* Plugged it in my PC, and the device presented itself as an USB microphone.
* Fired up Audacity, and set it for recording 44100 hz mono from the USB microphone device (the Cassette to MP3 Converter).
* Pressed record in Audacity, pressed Play on the Cassette player.
* Finished recording, exported audio to turtles-side1.wav

```bash

# Build ubercassette like so:
cd src
make
cd ..

# Used ubercassette like so:
./ubercassette turtles-side1.wav turtles-side1.tap
```

Easy!

# A note on my cassette player
I used a cheap "Cassette to MP3 Converter" it had a label "MODEL NO.: RC-2765" but it's from China, so I don't know if it means anything to anyone, but it is a silver-gray walkman style device, which takes 2 AA batteries, has a 3.5 mm Jack for headphones, a mini-usb plug, a volume control, playback-controls and a DC3V power connector (for which no AC adapter came). The player worked perfectly, and I did not have to clean up the audio before using it.

