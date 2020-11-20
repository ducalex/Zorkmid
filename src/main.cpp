#include "System.h"

bool done = false;

void END()
{
    done = true;
}

void UpdateGameSystem()
{
    ProcMTime();
    UpdateDTime();
    // message processing loop
    SDL_Event event;

    //Clear all hits
    FlushHits();
    while (SDL_PollEvent(&event))
    {
        // check for messages
        switch (event.type)
        {
            // exit if the window is closed
        case SDL_QUIT:
            done = true;
            break;

            // check for keyhit's (one per press)
        case SDL_KEYDOWN:
            SetHit(event.key.keysym.sym);
            break;
        }
    }
    //check for keydown (continous)
    UpdateKeyboard();

    FpsCounter();

    if ((KeyDown(SDLK_RALT) || KeyDown(SDLK_LALT)) && KeyHit(SDLK_RETURN))
        Rend_SwitchFullscreen();
}

int main(int argc, char **argv)
{
    InitFileManage();

    char buf[512];
    char buf2[512];
    bool fullscreen = false;
    const char *pa = "./";
    for (int i = 1; i < argc; i++)
    {
        if (strcasecmp(argv[i], "-f") == 0)
        {
            fullscreen = true;
        }
        else
        {
            pa = argv[i];
        }
    }

    SetAppPath(pa);

    sprintf(buf, "%s/%s", pa, "Zork.dir");
    FILE *fp = fopen(buf, "rb");

    if (!fp)
    {
        // If file not exists then use ZORK_DIR
        char *(dirs[]) = ZORK_DIR;

        // If read error then bail:
        printf("Can't open %s\n", buf);
        exit(1);
    }

    while (!feof(fp))
    {
        memset(buf, 0, 128);
        if (fgets(buf, 128, fp) == NULL)
            break;
        char *sstr = TrimRight(TrimLeft(buf));
        int sstr_l = strlen(sstr);
        if (sstr != NULL)
            if (sstr_l > 1)
            {
                for (int i = 0; i < sstr_l; i++)
                    if (sstr[i] == '\\')
                        sstr[i] = '/';

                sprintf(buf2, "%s/%s", pa, sstr);

                ListDir(buf2);
            }
    }

    fclose(fp);

    InitVkKeys();

    sprintf(buf, "%s/%s", pa, "FONTS");
    Rend_InitGraphics(fullscreen, buf);

    sprintf(buf, "%s/%s", pa, SYS_STRINGS_FILE);
    ReadSystemStrings(buf);

    menu_LoadGraphics();

    InitScriptsEngine();

    InitGameLoop();

    InitMTime(35.0);

    while (!done)
    {
        UpdateGameSystem();
        GameLoop();
    }

    SDL_SetGamma(1.0, 1.0, 1.0);

    SDL_Quit();

    return 0;
}
