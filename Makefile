CC = gcc
CXX = g++
MINGW = /usr/i686-w64-mingw32
CC_WIN = i686-w64-mingw32-gcc
CXX_WIN = i686-w64-mingw32-g++
LIBS = -Bstatic -lSDL -lSDL_image -lSDLmain -lSDL_mixer -lSDL_ttf
LIBS_WIN = -L$(MINGW)/lib/ -lmingw32  -lSDL  -lSDL_image -lSDLmain -lSDL_mixer -lSDL_ttf -lSDL.dll -mwindows
TARGET = Zorkmid
OBJS =  \
	src/Actions.c\
	src/Anims.c\
	src/Control.c\
	src/Game.c\
	src/Inventory.c\
	src/Loader.c\
	src/Menu.c\
	src/Mouse.c\
	src/main.c\
	src/mylist.c\
	src/Puzzle.c\
	src/Render.c\
	src/Scripting.c\
 	src/Sound.c\
	src/Subtitles.c\
	src/System.c\
	src/Text.c\
	src/avi_duck/simple_avi.c\
	src/avi_duck/truemotion1.c

linux: $(OBJS)
	$(CC) -std=c99 -O3 -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

linux_trace: $(OBJS)
	$(CC) -std=c99 -DTRACE -Og -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

win32_trace: $(OBJS)
	$(CC_WIN) -std=c99 -DTRACE -Og -o $(TARGET).exe $(OBJS) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

win32: $(OBJS)
	$(CC_WIN) -std=c99 -O3 -o $(TARGET).exe $(OBJS) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

clean:
	rm -f $(TARGET) $(TARGET_DINGOO) *.o