# Pico Reco
## Local Speech Recognition for Small Robots

This project provides low-resource streaming speech recognition using the Picovoice Cheetah system (free). It operates completely offline (no network) and is available as a [C++ class](src/jhcPicoReco.cpp) or a [Python wrapper](pico_reco.py). The code is known to work on Jetson Nano running Ubuntu 18.04 and Raspberry Pi 4B running Bullseye (but **not** Buster). 

All speech commands (as delimited by pauses) are transcribed with no special attention word. To use the system call the "start" function, loop using the "status" function, then pull result strings using the "heard" function. The simple program [test_reco](src/test_reco.cpp) shows how to do this (or see \_\_main\_\_ in the Python wrapper). Below is an example session:

    Configuring Picovoice and connecting to microphone ...
    --- Transcribing utterances for 20 secs ---
    
      What ...
      What color is the biggest ...
    What color is the biggest object on the table in the kitchen
    
    --- Done ---

The system runs in a background thread and uses the Linux pulseaudio front end (sorry, no Windows) to access the default microphone. The code is pretty vanilla except for the inclusion of a rough VOX circuit which skips speech recognition when things are quiet. This lets the system catch up on any lags induced by the highly variable delays in audio sampling and Picovoice calls.

## Configuration

This code needs a valid Picovoice key to operate! You can [sign up](https://console.picovoice.ai/signup) for free then copy your AccessKey string to file [picovoice.key](config/picovoice.key). 

The code here has been compiled for RPi4 and should be usable as is. If you want to build for Jetson Nano instead, change which library is commented out on line 42 of [CMakeLists.txt](CMakeLists.txt). To run the code you will also need pulseaudio installed:

    sudo apt-get install pulseaudio pavucontrol libasound2-dev

Finally, this repository has release v2.0 of the Picovoice Cheetah shared library and speech model.
The newest library for RPi is [Cortex A72 32 bit](https://github.com/Picovoice/cheetah/tree/master/lib/raspberry-pi/cortex-a72) 
and for Jetson is [Cortex A57 64 bit](https://github.com/Picovoice/cheetah/tree/master/lib/jetson/cortex-a57-aarch64). 
You might also want to check for an updated version of the run-time model [cheetah_params.pv](https://github.com/Picovoice/cheetah/tree/master/lib/common) on GitHub.

---

December 2023 - Jonathan Connell - jconnell@alum.mit.edu