# TOSLINK Audio Transport

This is a bespoke, truly-gapless, CD-quality, stereo audio player for the Teensy 4.1 platform!

The genesis of this project was just me sitting down with my [Teensy 4.1 prototyping system](https://protosupplies.com/product/prototyping-system-for-teensy-4-1/) and messing around with the included CD-quality audio playback demo sketch. This got me thinking: "How much work would it ACTUALLY take to turn this grunty lil' microprocessor dev board into a digital transport for my entire CD library? And could I get the output to end up as a 1:1, gapless, bit-accurate match of what gets passed through my portable CD player's optical output?!?" :eyes:

Well, a few quick internet searches showed me that this could be more than doable. It would ultimately just boil down to how much of the [pre-built library](https://www.pjrc.com/teensy/td_libs_Audio.html) would suit my needs and how much extra functionality would need to be built completely from scratch. So yeah, here are the "up-to-the-last-PR" results of this ongoing R&D! :grin:

## Hardware
The current hardware BoM is as follows:
- Teensy 4.1 dev board (WITH 16 MB OF PSRAM!!!)
- Hosyond 3.5" 480x320 IPS touchscreen display
- S/PDIF-to-TOSLINK converter (see below for more info on purchasing)
- A microSD card (up to 512 GB capacity tested and working so far)
- Any DAC which can decode a TOSLINK-compliant fiber optic signal
- A TOSLINK cable (with the correct termination style on either end, check your equipment before purchasing)
- Basic breadboarding supplies (linking two breadboards along the power rails creates a roomier setup with less fidling)

If you can lock down the above hardware, you can get this project replicated on your end!

### Teensy 4.1
Various configurations of Teensy 4.1 dev boards are relatively easy to find in-stock, but the very specific consideration for the current state of this project is that the board MUST have two identical 8 MB PSRAM chips soldered to the underside of the PCB. (Further tests will need to be conducted to see if other RAM configurations can be natively supported by this codebase.) This PSRAM is *absolutely essential*, as this is where the rolling audio buffer lives (more on this below in the technical breakdown). The correct board can be found [here](https://protosupplies.com/product/teensy-4-1-fully-loaded/) with the "16MB PSRAM" option in the dropdown menu being the one you're looking for, just in case you need a reference.

### Hosyond 3.5" 480x320 IPS Touchscreen Display
This codebase contains the correct configuration for [this exact panel](https://www.amazon.com/dp/B0CMD7Y55M?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1). No other displays are currently supported at this time, but this particular display makes the UI considerably more elevated than just a standard "messing with Arduino for the first time" type of display. The IPS technology ensures that colors stay strong and accurate when viewing the screen at extreme angles. Also, the slightly larger pixel count (vs. the entry-level 320x240 displays) combined with the capacitive touchscreen capability keeps the UX/UI nice and snappy without too much on-screen clutter!

### S/PDIF-to-TOSLINK Converter
This may be the hardest piece of hardware to lock down, depending on when you're reading this documentation. As of late-summer of 2026, I was able to easily locate exactly what I needed on AliExpress. [This exact module](https://www.aliexpress.us/item/3256808682814446.html?spm=a2g0o.productlist.main.23.1b0fc1mRc1mRcm&algo_pvid=5208b283-c0e5-4c13-bdc4-52074b8edc6e&algo_exp_id=5208b283-c0e5-4c13-bdc4-52074b8edc6e-22&pdp_ext_f=%7B%22order%22%3A%2214%22%2C%22eval%22%3A%221%22%2C%22fromPage%22%3A%22search%22%7D&pdp_npi=6%40dis%21USD%2123.82%2119.77%21%21%2123.82%2119.77%21%402101c28d17897669741815014e0f03%2112000047022763228%21sea%21US%210%21ABX%211%210%21n_tag%3A-29910%3Bd%3Aceca9d7b%3Bm03_new_user%3A-29895&curPageLogUid=do7nJx7ilR8o&utparam-url=scene%3Asearch%7Cquery_from%3A%7Cx_object_id%3A1005008869129198%7C_p_origin_prod%3A#nav-store) did the trick perfectly and I opted for the "upgrade version" (even though there shouldn't be a difference in audio quality, theoretically-speaking). At its core, it's just a small utility board which can accept a S/PDIF-compliant electrical signal (the two-channel LPCM audio data streaming from pin 14 of the Teensy) and convert it to both coaxial and optical digital outputs. I'm not sure if anyone makes a board which ONLY converts the signal to fiber optic (as that would save a bit of space), but the board linked above works so well that I simply heard audio coming out of my DAC the moment I wired this board into this system. No messy config needed!

### MicroSD Card
Self-explanatory! I've tested various card sizes up to 512 GB so far and the system's RAM buffer didn't have any trouble retrieving the raw audio data well before the playback system needed it for the S/PDIF conversion handshake. So yeah, as long as your tracks are 16-bit/44.1kHz ".wav" files (CD-quality rips, basically), go nuts with however much music you'd like add onto your card!

### The DAC and TOSLINK Cable
The signal that this setup ultimately produces is referenced [here](https://en.wikipedia.org/wiki/TOSLINK). All you will need in order to hear your music come to life in glorious, gapless CD-quality is a TOSLINK cable (which carries the light pulses) and any commercially-made DAC which can accept a TOSLINK-compliant signal and decode it. I used [this one](https://www.smsl-audio.com/portal/product/detail/id/870.html) when beginning this project, as it is quite portable, powered through a USB-C connection, and ultimately just sounds great!

## "What's under the hood?!?"

(Under construction until a bit more aural testing and code hardening can be completed...)

## Preparing Your Audio Files

Given that this started as a purely-personal project, my starting point was my sizable library of CD-quality "bin/cue"-style backups of most of my legacy CD collection that I've had bouncing around on one of my hard drives. Through the years, I've been using [Exact Audio Copy](https://www.exactaudiocopy.de/) for archiving my digital music library and I've always gotten stellar results. Given that the full-album, cue-file-based scanning method was incompatible with the Teensy audio library (in its base state, at least), I realized that I was going to have to split my album-long ".wav" archival files into individual tracks. (Note to future Ryan: Don't forget to leave out the explanation as to why you landed on the individual track method when fleshing out the above technical section!)

Thankfully, EAC has an easy, automated way to do this built right into the program! If you select (list menu selections here), it opens two sequential "open file" windows. The first window is where you will select the album-long ".wav" audio file and the second window is where you will select the album's corresponding ".cue" file. EAC with then begin the process of splitting your archival CD image into individual tracks based on the breakpoints found in the ".cue" file.

Why is it so important to follow my method (for now, at least)? Well, for one, you get the incredibly-forward-thinking side effect of creating an archival-grade backup of your physical media! But from a more purely-technical standpoint, I haven't stress-tested the audio engine against other file creation systems. It achieves 100% gapless playback when using individual tracks created by EAC's internal processes, but I can't be sure that other file creation systems are truncating ".wav" bitstreams in EXACTLY the same fashion. This project's firmware includes dynamic ".wav" file *header* parsing, but there's a small possibility that other systems/software could add even just few extra bits/bytes at the very end of every track file. The audio engine wouldn't know to lop these bits off the end (in the way that it automatically does with the header data) and would, therefore, just pass this garbage data directly into RAM *thinking* that it was doing its job of ensuring 100% gapless playback.

At the end of the day, my real-world aural testing has led me to listen to a number of different albums where sound flows over the track delineation line (i.e. track A flows seamlessly into track B on the physical CD). Not a single album has had even the tiniest audio glitch/blip/pop/whathaveyou when crossing the threshold between tracks when playing through this firmware's playback engine. The code is pretty solid by this point, but we can't discount the nature of how the files have been prepared as well. Just a word of warning!!!

In order to get the file system to recognize your music, you MUST follow the artist folder --> album folder --> track file(s) organizational scheme on your SD card. For now, the first two menu pages are hardcoded to only recognize artist/album folders and all of the track sorting happens in concert with how EAC names the new ".wav" files when it performs the split. For now, the way to do it is to have a two-digit number as the first two characters of the filename, then a space, then the actual track name followed by ".wav". Here's an example:

"05 Song Name.wav"

This exact format will tell the file sorting system that it's a chunk of data that should be treated as audio information and that it's the fifth track on the album.

(More README to come as things continue to approach a 1.0.0 release!!!)
