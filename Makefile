CC = gcc
CXX = g++
MINGW = /usr/i686-w64-mingw32
CC_WIN = i686-w64-mingw32-gcc
CXX_WIN = i686-w64-mingw32-g++
LIBS = -Bstatic -lSDL -lSDL_gfx -lSDLmain -lSDL_mixer -lSDL_ttf
LIBS_WIN = -L$(MINGW)/lib/ -lmingw32  -lSDL -lSDL_gfx -lSDLmain -lSDL_mixer -lSDL_ttf -lSDL.dll -mwindows
TARGET = Zorkmid
OBJS =  \
	src/Actions.c\
	src/Anims.c\
	src/Controls.c\
	src/Game.c\
	src/Inventory.c\
	src/Loader.c\
	src/Menu.c\
	src/Mouse.c\
	src/main.c\
	src/Puzzle.c\
	src/Render.c\
	src/Scripting.c\
 	src/Sound.c\
	src/System.c\
	src/Text.c\
	src/Timer.c\
	src/Decoder.c\
	src/codecs/truemotion1.c

linux: $(OBJS)
	$(CC) -std=c99 -Wall -O3 -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

win32: $(OBJS)
	$(CC_WIN) -std=c99 -Wall -O3 -o $(TARGET).exe $(OBJS) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

linux_debug: $(OBJS)
	$(CC) -std=c99 -DENABLE_TRACING -Wall -Og -pg -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

win32_debug: $(OBJS)
	$(CC_WIN) -std=c99 -DENABLE_TRACING -Wall -Og -pg -o $(TARGET).exe $(OBJS) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

clean:
	rm -f $(TARGET) *.o