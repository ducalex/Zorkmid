#include "System.h"

int main(int argc, char **argv)
{
    char buf[512];
    char buf2[512];
    bool fullscreen = false;
    bool widescreen = true;
    const char *pa = "./";

    for (int i = 1; i < argc; i++)
    {
        if (strcasecmp(argv[i], "-f") == 0)
        {
            fullscreen = true;
        }
        else if (strcasecmp(argv[i], "-nocrop") == 0)
        {
            widescreen = false;
        }
        else if (strcasecmp(argv[i], "-zgi") == 0)
        {
            // CUR_GAME = GAME_ZGI;
        }
        else if (strcasecmp(argv[i], "-nem") == 0)
        {
            // CUR_GAME = GAME_NEM;
        }
        else
        {
            pa = argv[i];
        }
    }

    Rend_InitGraphics(fullscreen, widescreen);
    InitSound();
    InitFileManager(pa);
    Mouse_LoadCursors();
    menu_LoadGraphics();
    InitVkKeys();
    GameInit();

    while (true)
    {
        GameUpdate();
        GameLoop();
    }

    GameQuit();

    return 0;
}
