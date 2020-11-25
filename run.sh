#!/bin/sh
make win32 || exit
cp -f Zorkmid.exe ~/Games/Zork\ Grand\ Inquisitor/Zorkmid.exe
cd ~/Games/Zork\ Grand\ Inquisitor
./Zorkmid.exe

