# Description

Technological demo featuring various areas as using OpenGL ES with Android NDK, custom message queues, interacting JNI and Java activities, using NORAD databases and parsing their data.

Features:
 - Edit
 - C++11 with clang compiler
 - OpenGL ES 2
 - Multitouch for scaling, rotating and moving
 - NORAD TLE database
 - SGP4/SDP4 orbital model

# Installation instructions

Shell commands:

    $ cd google-play-services_lib/
    $ android update project -p .
    $ ant debug
    $ cd ../v7-appcompat_lib/
    $ android update project -p .
    $ ant debug
    $ cd ../glSatellite/
    $ android update project -p .
    $ ndk-build
    $ ant debug install

# TODO

 - test offline mode
 - fix logging messages in Java
 - star.bmp -> png

# References

The Official Khronos WebGL Repository: https://github.com/KhronosGroup/WebGL
NASA Earth textures: http://earthobservatory.nasa.gov/Features/BlueMarble/
NORAD Two-Line Element Sets Current Data: http://www.celestrak.com/NORAD/elements/
Real Time Satellite Tracking And Predictions: http://www.n2yo.com/
