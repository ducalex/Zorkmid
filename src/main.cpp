#include "System.h"

int main(int argc, char **argv)
{
    InitFileManager();

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

    SetAppPath(pa);

    sprintf(buf, "%s/%s", pa, "Zork.dir");
    FILE *fp = fopen(buf, "rb");

    if (!fp)
    {
        // If file not exists then use ZORK_DIR
        // char *(dirs[]) = ZORK_DIR;

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

    GameInit(fullscreen);

    while (true)
    {
        GameUpdate();
        GameLoop();
    }

    GameQuit();

    return 0;
}
