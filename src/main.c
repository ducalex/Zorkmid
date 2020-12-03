#include "System.h"

int main(int argc, char **argv)
{
    bool fullscreen = false;
    const char *path = "./";

    for (int i = 1; i < argc; i++)
    {
        if (str_equals(argv[i], "-f"))
        {
            fullscreen = true;
        }
        else if (argv[i][0] == '-')
        {
            // Unknown option
        }
        else
        {
            path = argv[i];
        }
    }

    Game_Init(path, fullscreen);

    while (true)
    {
        Game_Update();
        Game_Loop();
    }

    Game_Quit();

    return 0;
}
