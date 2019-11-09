#!/bin/bash
rm a.out
g++ *.cpp -lGL -lGLU -lglfw -lSDL -lSDL_image -lGLEW
./a.out

