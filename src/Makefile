CC = gcc
CXX = g++
MINGW = /usr/i686-w64-mingw32
CC_WIN = i686-w64-mingw32-gcc
CXX_WIN = i686-w64-mingw32-g++
LIBS = -Bstatic -lSDL -lSDL_image -lSDL_gfx -lSDLmain -lSDL_mixer -lSDL_ttf
LIBS_WIN = -L$(MINGW)/lib/ -lmingw32  -lSDL  -lSDL_image -lSDL_gfx -lSDLmain -lSDL_mixer -lSDL_ttf -lSDL.dll -mwindows
TARGET = Zengine
OBJS =  \
	Actions.cpp\
	Control.cpp\
	Game.cpp\
 	Sound.cpp\
	main.cpp\
	System.cpp\
	Mouse.cpp\
	mylist.cpp\
	Puzzle.cpp\
	Render.cpp\
	ScriptSystem.cpp\
	Timers.cpp\
	Anims.cpp\
	Menu.cpp\
	Subtitles.cpp\
	Text.cpp\
	Loader.cpp\
	Inventory.cpp\
	avi_duck/simple_avi.cpp\
	avi_duck/truemotion1.cpp

WINOBJ =

linux: $(OBJS)
	$(CXX) -Wno-write-strings -O3 -Os -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/
	$(CXX) -Wno-write-strings -DWIDESCREEN -O3 -Os -o $(TARGET)_wide $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

linux_trace: $(OBJS)
	$(CXX) -Wno-write-strings -DTRACE -g -o $(TARGET) $(OBJS) $(LIBS) -I /usr/include/ -I /usr/include/SDL/

win32_trace: $(OBJS) $(WINOBJ)
	$(CXX_WIN) -Wno-write-strings -DTRACE -O3 -Os -o $(TARGET).exe $(OBJS) $(WINOBJ) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

win32: $(OBJS) $(WINOBJ)
	# $(CXX_WIN) -Wno-write-strings -Os -o $(TARGET).exe $(OBJS) $(WINOBJ) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/
	$(CXX_WIN) -Wno-write-strings -DWIDESCREEN -Os -o $(TARGET)_wide.exe $(OBJS) $(WINOBJ) $(LIBS_WIN) -I $(MINGW)/include/ -I $(MINGW)/include/SDL/

clean:
	rm -f $(TARGET) $(TARGET_DINGOO) *.o