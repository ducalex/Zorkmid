#!/bin/sh
make -j win32_debug || exit
cp -f Zorkmid.exe ~/Games/Zork\ Grand\ Inquisitor/Zorkmid.exe
cd ~/Games/Zork\ Grand\ Inquisitor
gdb ./Zorkmid.exe --eval-command=run
gprof.exe ./Zorkmid.exe > profile.txt
