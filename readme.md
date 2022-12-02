A collection of audio effects writen in C using the jack-audio api.

Build using "gcc -o file_name file_name.c `pkg-config --cflags --libs jack`"

For using the compiled exe, you have to start jack on your linux system and run ./file_name

The projects in this repository are very much inspired by the https://github.com/jackaudio/example-clients

1.simple_echo_fx.c - is a very simple echo effect implementation.