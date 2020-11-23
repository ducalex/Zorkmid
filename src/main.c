#include "System.h"

int main(int argc, char **argv)
{
    bool fullscreen = false;
    bool widescreen = false;
    const char *path = "./";

    for (int i = 1; i < argc; i++)
    {
        if (str_equals(argv[i], "-f"))
        {
            fullscreen = true;
        }
        else if (str_equals(argv[i], "-w"))
        {
            widescreen = true;
        }
        else if (str_equals(argv[i], "-zgi"))
        {
            // CUR_GAME = GAME_ZGI;
        }
        else if (str_equals(argv[i], "-nem"))
        {
            // CUR_GAME = GAME_NEM;
        }
        else
        {
            path = argv[i];
        }
    }

    Rend_InitGraphics(fullscreen, widescreen);
    Sound_Init();
    InitVkKeys();
    GameInit(path);

    while (true)
    {
        GameUpdate();
        GameLoop();
    }

    GameQuit();

    return 0;
}
