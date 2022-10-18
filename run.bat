echo off
g++ src/main.cpp -o arkanoid.exe -Iinclude -L/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio 
start arkanoid.exe