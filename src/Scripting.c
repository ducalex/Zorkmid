#include "System.h"

typedef struct
{
    puzzlenode_t *nod[PuzzleStack];
    int32_t cnt;
} StateBoxEnt_t;

static int32_t VAR_SLOTS_MAX = 20000;
static int32_t gVars[30000];
static action_res_t *gNodes[30000];
static uint8_t Flags[30000];
static StateBoxEnt_t *StateBox[30000];

static const char *PreferencesFile = NULL;

static bool BreakExecute = false;

static pzllst_t *uni = NULL;   //universe script
static pzllst_t *world = NULL; //world script
static pzllst_t *room = NULL;  //room script
static pzllst_t *view = NULL;  //view script

static MList *controls = NULL;   // Controls
static MList *actions = NULL; //sounds, animations, ttytexts and other.

static uint8_t SaveBuffer[512 * 1024];
static uint32_t SaveCurrentSize = 0;

void FillStateBoxFromList(pzllst_t *lst);
void ShakeStateBox(uint32_t indx);

pzllst_t *GetUni()
{
    return uni;
}

pzllst_t *Getworld()
{
    return world;
}

pzllst_t *Getroom()
{
    return room;
}

pzllst_t *Getview()
{
    return view;
}

MList *GetControlsList()
{
    return controls;
}

MList *GetActionsList()
{
    return actions;
}

void ScrSys_AddToActionsList(void *nod)
{
    if (actions && nod)
        AddToMList(actions, nod);
}

void SetgVarInt(uint32_t indx, int var)
{
    if (indx < VAR_SLOTS_MAX)
    {
        gVars[indx] = var;
        ShakeStateBox(indx);
    }
}

void SetDirectgVarInt(uint32_t indx, int var)
{
    if (indx < VAR_SLOTS_MAX)
        gVars[indx] = var;
}

int GetgVarInt(uint32_t indx)
{
    if (indx < VAR_SLOTS_MAX)
        return gVars[indx];
    return 0;
}

int *GetgVarRef(uint32_t indx)
{
    if (indx < VAR_SLOTS_MAX)
        return &gVars[indx];
    return NULL;
}

uint8_t ScrSys_GetFlag(uint32_t indx)
{
    if (indx < VAR_SLOTS_MAX)
        return Flags[indx];
    return 0;
}

void ScrSys_SetFlag(uint32_t indx, uint8_t newval)
{
    if (indx < VAR_SLOTS_MAX)
        Flags[indx] = newval;
}

action_res_t *GetGNode(uint32_t indx)
{
    if (indx < VAR_SLOTS_MAX)
        return gNodes[indx];
    return NULL;
}

void SetGNode(uint32_t indx, action_res_t *data)
{
    if (indx < VAR_SLOTS_MAX)
        gNodes[indx] = data;
}

void ScrSys_Init()
{
    if (CUR_GAME == GAME_ZGI)
    {
        PreferencesFile = "./prefs_zgi.ini";
        VAR_SLOTS_MAX = 20000;
    }
    else
    {
        PreferencesFile = "./prefs_znem.ini";
        VAR_SLOTS_MAX = 30000;
    }

    memset(StateBox, 0x0, sizeof(StateBox));
    memset(Flags, 0x0, sizeof(Flags));
    memset(gVars, 0x0, sizeof(gVars));
    memset(gNodes, 0x0, sizeof(gNodes));

    view = Puzzle_CreateList("view");
    room = Puzzle_CreateList("room");
    world = Puzzle_CreateList("world");
    uni = Puzzle_CreateList("universe");
    controls = CreateMList();
    actions = CreateMList();

    //needed for znemesis
    SetDirectgVarInt(SLOT_CPU, 1);
    SetDirectgVarInt(SLOT_PLATFORM, 0);
    SetDirectgVarInt(SLOT_WIN958, 0);

    ScrSys_LoadPreferences();
}

void ScrSys_LoadScript(pzllst_t *lst, const char *filename, bool control, MList *controlst)
{
    mfile_t *fl = mfopen(filename);
    if (!fl)
        Z_PANIC("Unable to load script file '%s'\n", filename);

    LOG_DEBUG("Loading script file '%s'\n", filename);

    if (control)
        Rend_SetRenderer(RENDER_FLAT);

    char buf[STRBUFSIZE];

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);

        char *str = PrepareString(buf);

        if (str_starts_with(str, "puzzle"))
        {
            Puzzle_Parse(lst, fl, str);
        }
        else if (str_starts_with(str, "control") && control)
        {
            Control_Parse(controlst, fl, str);
        }
    }

    mfclose(fl);
}

void ScrSys_ClearStateBox()
{
    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        if (StateBox[i] != NULL)
            DELETE(StateBox[i]);
        StateBox[i] = NULL;
    }
}

void ScrSys_PrepareSaveBuffer()
{
    int buffpos = 0;

    SaveBuffer[0] = 'Z';
    SaveBuffer[1] = 'N';
    SaveBuffer[2] = 'S';
    SaveBuffer[3] = 'G';
    SaveBuffer[4] = 4;
    SaveBuffer[5] = 0;
    SaveBuffer[6] = 0;
    SaveBuffer[7] = 0;
    SaveBuffer[12] = 'L';
    SaveBuffer[13] = 'O';
    SaveBuffer[14] = 'C';
    SaveBuffer[15] = ' ';
    SaveBuffer[16] = 8;
    SaveBuffer[17] = 0;
    SaveBuffer[18] = 0;
    SaveBuffer[19] = 0;
    SaveBuffer[20] = tolower(GetgVarInt(SLOT_WORLD));
    SaveBuffer[21] = tolower(GetgVarInt(SLOT_ROOM));
    SaveBuffer[22] = tolower(GetgVarInt(SLOT_NODE));
    SaveBuffer[23] = tolower(GetgVarInt(SLOT_VIEW));

    int16_t *tmp2 = (int16_t *)&SaveBuffer[24];
    *tmp2 = GetgVarInt(SLOT_VIEW_POS);

    buffpos = 28;

    MList *lst = GetActionsList();
    StartMList(lst);
    while (!EndOfMList(lst))
    {
        action_res_t *nod = (action_res_t *)DataMList(lst);
        if (nod->node_type == NODE_TYPE_TIMER)
        {
            SaveBuffer[buffpos] = 'T';
            SaveBuffer[buffpos + 1] = 'I';
            SaveBuffer[buffpos + 2] = 'M';
            SaveBuffer[buffpos + 3] = 'R';
            SaveBuffer[buffpos + 4] = 8;
            SaveBuffer[buffpos + 5] = 0;
            SaveBuffer[buffpos + 6] = 0;
            SaveBuffer[buffpos + 7] = 0;

            int32_t *tmp = (int32_t *)&SaveBuffer[buffpos + 8];
            *tmp = nod->slot;

            tmp = (int32_t *)&SaveBuffer[buffpos + 12];

            *tmp = nod->nodes.node_timer;

            buffpos += 16;
        }

        NextMList(lst);
    }

    SaveBuffer[buffpos] = 'F';
    SaveBuffer[buffpos + 1] = 'L';
    SaveBuffer[buffpos + 2] = 'A';
    SaveBuffer[buffpos + 3] = 'G';

    buffpos += 4;

    int32_t *tmp = (int32_t *)&SaveBuffer[buffpos];

    *tmp = VAR_SLOTS_MAX * 2; //16bits

    buffpos += 4;

    tmp2 = (int16_t *)&SaveBuffer[buffpos];

    for (int i = 0; i < VAR_SLOTS_MAX; i++)
        tmp2[i] = ScrSys_GetFlag(i);

    buffpos += VAR_SLOTS_MAX * 2;

    SaveBuffer[buffpos] = 'P';
    SaveBuffer[buffpos + 1] = 'U';
    SaveBuffer[buffpos + 2] = 'Z';
    SaveBuffer[buffpos + 3] = 'Z';

    buffpos += 4;

    tmp = (int32_t *)&SaveBuffer[buffpos];

    *tmp = VAR_SLOTS_MAX * 2; //16bits

    buffpos += 4;

    tmp2 = (int16_t *)&SaveBuffer[buffpos];

    for (int i = 0; i < VAR_SLOTS_MAX; i++)
        tmp2[i] = GetgVarInt(i);

    lst = GetActionsList();
    StartMList(lst);
    while (!EndOfMList(lst))
    {
        action_res_t *nod = (action_res_t *)DataMList(lst);
        if (nod->node_type == NODE_TYPE_MUSIC)
            if (nod->slot > 0)
                tmp2[nod->slot] = 2;

        NextMList(lst);
    }

    buffpos += VAR_SLOTS_MAX * 2;

    SaveCurrentSize = buffpos;
}

void ScrSys_SaveGame(char *file)
{
    if (SaveBuffer[0] != 'Z')
        return;

    FILE *f = fopen(file, "wb");
    fwrite(SaveBuffer, SaveCurrentSize, 1, f);
    fclose(f);
}

void ScrSys_LoadGame(char *file)
{
    uint32_t tmp;
    int16_t tmp2;
    int32_t slot, time;
    uint8_t w, r, n, v;
    int16_t pos;
    char buf[32];

    mfile_t *f = mfopen(file);
    if (!f)
        return;

    ScrSys_FlushActionsList();

    mfread(&tmp, 4, f);
    if (tmp != MAGIC_SAV)
        Z_PANIC("Invalid save file '%s' (MAGIC)\n", file);

    mfread(&tmp, 4, f);
    mfread(&tmp, 4, f);
    mfread(&tmp, 4, f);
    mfread(&tmp, 4, f);

    mfread(&w, 1, f);
    mfread(&r, 1, f);
    mfread(&n, 1, f);
    mfread(&v, 1, f);

    mfread(&pos, 2, f);

    mfread(&tmp, 2, f);

    while (!mfeof(f))
    {
        mfread(&tmp, 4, f);
        if (tmp != 0x524D4954)
            break;

        mfread(&tmp, 4, f);

        mfread(&slot, 4, f);
        mfread(&time, 4, f);

        sprintf(buf, "%d", time / 100);
        Actions_Run("timer", buf, slot, view);
    }

    mfread(&tmp, 4, f);

    if (tmp != VAR_SLOTS_MAX * 2)
        Z_PANIC("Error in save file %s (FLAGS VAR_SLOTS_MAX)\n", file);

    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        mfread(&tmp2, 2, f);
        ScrSys_SetFlag(i, tmp2);
    }

    mfread(&tmp, 4, f);
    mfread(&tmp, 4, f);

    if (tmp != VAR_SLOTS_MAX * 2)
        Z_PANIC("Error in save file %s (PUZZLE VAR_SLOTS_MAX)\n", file);

    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        mfread(&tmp2, 2, f);
        SetDirectgVarInt(i, tmp2);
    }

    Rend_SetDelay(2);

    ScrSys_ChangeLocation(w, r, n, v, pos, true);

    SetgVarInt(SLOT_JUST_RESTORED, 1);

    mfclose(f);

    ScrSys_LoadPreferences();
}

void ScrSys_ChangeLocation(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X, bool force_all) // world / room / view
{
    Location_t temp;
    temp.World = w;
    temp.Room = r;
    temp.Node = v1;
    temp.View = v2;
    temp.X = X;

    if (GetgVarInt(SLOT_WORLD) != SystemWorld || GetgVarInt(SLOT_ROOM) != SystemRoom)
    {
        if (temp.World == SystemWorld && temp.Room == SystemRoom)
        {
            SetDirectgVarInt(SLOT_MENU_LASTWORLD, GetgVarInt(SLOT_WORLD));
            SetDirectgVarInt(SLOT_MENU_LASTROOM, GetgVarInt(SLOT_ROOM));
            SetDirectgVarInt(SLOT_MENU_LASTNODE, GetgVarInt(SLOT_NODE));
            SetDirectgVarInt(SLOT_MENU_LASTVIEW, GetgVarInt(SLOT_VIEW));
            SetDirectgVarInt(SLOT_MENU_LASTVIEW_POS, GetgVarInt(SLOT_VIEW_POS));
        }
        else
        {
            SetDirectgVarInt(SLOT_LASTWORLD, GetgVarInt(SLOT_WORLD));
            SetDirectgVarInt(SLOT_LASTROOM, GetgVarInt(SLOT_ROOM));
            SetDirectgVarInt(SLOT_LASTNODE, GetgVarInt(SLOT_NODE));
            SetDirectgVarInt(SLOT_LASTVIEW, GetgVarInt(SLOT_VIEW));
            SetDirectgVarInt(SLOT_LASTVIEW_POS, GetgVarInt(SLOT_VIEW_POS));
        }
    }

    ScrSys_ClearStateBox();

    if (temp.World == SaveWorld && temp.Room == SaveRoom &&
        temp.Node == SaveNode && temp.View == SaveView)
    {
        ScrSys_PrepareSaveBuffer();
    }

    char buf[32];
    char tm[5];

    if (temp.View != GetgVarInt(SLOT_VIEW) ||
        temp.Node != GetgVarInt(SLOT_NODE) ||
        temp.Room != GetgVarInt(SLOT_ROOM) ||
        temp.World != GetgVarInt(SLOT_WORLD) ||
        force_all || view == NULL)
    {
        ScrSys_FlushResourcesByOwner(view);

        Puzzle_FlushList(view);
        Controls_FlushList(controls);

        tm[0] = temp.World;
        tm[1] = temp.Room;
        tm[2] = temp.Node;
        tm[3] = temp.View;
        tm[4] = 0;
        sprintf(buf, "%s.scr", tm);

        ScrSys_LoadScript(view, buf, true, controls);
    }

    if (temp.Room != GetgVarInt(SLOT_ROOM) ||
        temp.World != GetgVarInt(SLOT_WORLD) ||
        force_all || room == NULL)
    {
        ScrSys_FlushResourcesByOwner(room);

        Puzzle_FlushList(room);

        tm[0] = temp.World;
        tm[1] = temp.Room;
        tm[2] = 0;
        sprintf(buf, "%s.scr", tm);

        ScrSys_LoadScript(room, buf, false, NULL);
    }

    if (temp.World != GetgVarInt(SLOT_WORLD) || force_all || world == NULL)
    {
        ScrSys_FlushResourcesByOwner(world);

        Puzzle_FlushList(world);

        tm[0] = temp.World;
        tm[1] = 0;
        sprintf(buf, "%s.scr", tm);

        ScrSys_LoadScript(world, buf, false, NULL);

        Mouse_ShowCursor();
    }

    FillStateBoxFromList(uni);
    FillStateBoxFromList(view);
    FillStateBoxFromList(room);
    FillStateBoxFromList(world);

    SetgVarInt(SLOT_WORLD, w);
    SetgVarInt(SLOT_ROOM, r);
    SetgVarInt(SLOT_NODE, v1);
    SetgVarInt(SLOT_VIEW, v2);
    SetgVarInt(SLOT_VIEW_POS, X);

    Menu_SetVal(0xFFFF);

    BreakExecute = false;
}

void AddPuzzleToStateBox(int slot, puzzlenode_t *pzlnd)
{
    StateBoxEnt_t *ent = StateBox[slot];

    if (ent == NULL)
    {
        ent = NEW(StateBoxEnt_t);
        StateBox[slot] = ent;
        ent->cnt = 0;
    }
    if (ent->cnt < PuzzleStack)
    {
        ent->nod[ent->cnt] = pzlnd;
        ent->cnt++;
    }
}

void FillStateBoxFromList(pzllst_t *lst)
{
    StartMList(lst->_list);
    while (!EndOfMList(lst->_list))
    {
        puzzlenode_t *pzlnod = (puzzlenode_t *)DataMList(lst->_list);

        AddPuzzleToStateBox(pzlnod->slot, pzlnod);

        StartMList(pzlnod->CritList);
        while (!EndOfMList(pzlnod->CritList))
        {
            MList *CriteriaLst = (MList *)DataMList(pzlnod->CritList);

            int prevslot = 0;
            StartMList(CriteriaLst);
            while (!EndOfMList(CriteriaLst))
            {
                crit_node_t *crtnod = (crit_node_t *)DataMList(CriteriaLst);

                if (prevslot != crtnod->slot1)
                    AddPuzzleToStateBox(crtnod->slot1, pzlnod);

                prevslot = crtnod->slot1;

                NextMList(CriteriaLst);
            }

            NextMList(pzlnod->CritList);
        }
        NextMList(lst->_list);
    }
}

void AddStateBoxToStk(puzzlenode_t *pzl)
{
    pzllst_t *owner = pzl->owner;
    if (owner->stksize < PuzzleStack)
    {
        if (owner->stksize > 0)
        {
            int32_t numb = 0;
            for (int32_t i = owner->stksize - 1; i >= 0 && owner->stack[i] != NULL; i--)
            {
                if (owner->stack[i] == pzl)
                {
                    if (numb < MaxPuzzlesInStack)
                        numb++;
                    else
                        return;
                }
            }
        }

        owner->stack[owner->stksize] = pzl;
        owner->stksize++;
    }
    else
    {
        LOG_WARN("Can't add pzl# %d to Stack\n", pzl->slot);
    }
}

void ShakeStateBox(uint32_t indx)
{
    //Nemesis don't use statebox, but this engine does, well make for nemesis it non revert.
    if (!StateBox[indx])
        return;

    if (CUR_GAME == GAME_NEM)
    {
        for (int i = 0; i < StateBox[indx]->cnt; i++)
            AddStateBoxToStk(StateBox[indx]->nod[i]);
    }
    else
    {
        for (int i = StateBox[indx]->cnt - 1; i >= 0; i--)
            AddStateBoxToStk(StateBox[indx]->nod[i]);
    }
}

void ScrSys_ExecPuzzleList(pzllst_t *lst)
{
    if (lst->exec_times < 2)
    {
        StartMList(lst->_list);
        while (!EndOfMList(lst->_list))
        {
            if (Puzzle_TryExec((puzzlenode_t *)DataMList(lst->_list)) == ACTION_BREAK)
            {
                BreakExecute = true;
                break;
            }
            NextMList(lst->_list);
        }
        lst->exec_times++;
    }
    else
    {
        int i = 0, j = lst->stksize;

        while (i < j)
        {
            puzzlenode_t *to_exec = lst->stack[i];

            lst->stack[i] = NULL;

            if (Puzzle_TryExec(to_exec) == ACTION_BREAK)
            {
                BreakExecute = true;
                break;
            }

            i++;
        }

        int z = 0;
        for (i = j; i < lst->stksize; i++)
        {
            lst->stack[z] = lst->stack[i];
            z++;
        }
        lst->stksize = z;
    }
}

bool ScrSys_BreakExec()
{
    return BreakExecute;
}

void ScrSys_SetBreak()
{
    BreakExecute = true;
}

void ScrSys_ProcessActionsList()
{
    MList *lst = GetActionsList();

    int result = NODE_RET_OK;

    StartMList(lst);
    while (!EndOfMList(lst))
    {
        action_res_t *nod = (action_res_t *)DataMList(lst);

        nod->first_process = true;

        if (!nod->need_delete)
        {
            switch (nod->node_type)
            {
            case NODE_TYPE_ANIMPLAY:
            case NODE_TYPE_ANIMPRE:
            case NODE_TYPE_ANIMPRPL:
                result = Anim_ProcessNode(nod);
                break;
            case NODE_TYPE_MUSIC:
            case NODE_TYPE_SYNCSND:
            case NODE_TYPE_PANTRACK:
                result = Sound_ProcessNode(nod);
                break;
            case NODE_TYPE_TIMER:
                result = Timer_ProcessNode(nod);
                break;
            case NODE_TYPE_TTYTEXT:
                result = Text_ProcessTTYText(nod);
                break;
            case NODE_TYPE_DISTORT:
                result = Rend_ProcessNode(nod);
                break;
            case NODE_TYPE_REGION:
                result = NODE_RET_OK;
                break;
            default:
                result = NODE_RET_OK;
                break;
            };
        }
        else
        {
            result = NODE_RET_DELETE;
            ScrSys_DeleteActionNode(nod);
        }

        if (result == NODE_RET_DELETE)
            DeleteCurrentMList(lst);

        NextMList(lst);
    }
}

int ScrSys_DeleteActionNode(action_res_t *nod)
{
    switch (nod->node_type)
    {
    case NODE_TYPE_ANIMPLAY:
    case NODE_TYPE_ANIMPRE:
    case NODE_TYPE_ANIMPRPL:
        return Anim_DeleteNode(nod);
    case NODE_TYPE_MUSIC:
    case NODE_TYPE_SYNCSND:
    case NODE_TYPE_PANTRACK:
        return Sound_DeleteNode(nod);
    case NODE_TYPE_TIMER:
        return Timer_DeleteNode(nod);
    case NODE_TYPE_TTYTEXT:
        return Text_DeleteTTYText(nod);
    case NODE_TYPE_DISTORT:
    case NODE_TYPE_REGION:
        return Rend_DeleteNode(nod);
    default:
        return NODE_RET_NO;
    }
}

void ScrSys_FlushActionsList()
{
    MList *all = GetActionsList();

    StartMList(all);
    while (!EndOfMList(all))
    {
        ScrSys_DeleteActionNode((action_res_t *)DataMList(all));
        NextMList(all);
    }
    FlushMList(all);
}

void ScrSys_FlushResourcesByOwner(pzllst_t *owner)
{
    MList *all = GetActionsList();

    StartMList(all);
    while (!EndOfMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->owner == owner)
        {
            int result = NODE_RET_OK;

            if (nod->node_type == NODE_TYPE_MUSIC)
            {
                if (nod->nodes.node_music->universe == false)
                    result = Sound_DeleteNode(nod);
            }
            else
                result = ScrSys_DeleteActionNode(nod);

            if (result == NODE_RET_DELETE)
                DeleteCurrentMList(all);
        }

        NextMList(all);
    }
}

void ScrSys_FlushResourcesByType(int type)
{
    MList *all = GetActionsList();

    StartMList(all);
    while (!EndOfMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->node_type == type && nod->first_process == true)
            if (ScrSys_DeleteActionNode(nod) == NODE_RET_DELETE)
                DeleteCurrentMList(all);

        NextMList(all);
    }
}

static const struct
{
    const char *name;
    int slot;
} prefs[] =
    {
        {"KeyboardTurnSpeed", SLOT_KBD_ROTATE_SPEED},
        {"PanaRotateSpeed", SLOT_PANAROTATE_SPEED},
        {"QSoundEnabled", SLOT_QSOUND_ENABLE},
        {"VenusEnabled", SLOT_VENUSENABLED},
        {"HighQuality", SLOT_HIGH_QUIALITY},
        {"Platform", SLOT_PLATFORM},
        {"InstallLevel", SLOT_INSTALL_LEVEL},
        {"CountryCode", SLOT_COUNTRY_CODE},
        {"CPU", SLOT_CPU},
        {"MovieCursor", SLOT_MOVIE_CURSOR},
        {"NoAnimWhileTurning", SLOT_TURN_OFF_ANIM},
        {"Win958", SLOT_WIN958},
        {"ShowErrorDialogs", SLOT_SHOWERRORDIALOG},
        {"ShowSubtitles", SLOT_SUBTITLE_FLAG},
        {"DebugCheats", SLOT_DEBUGCHEATS},
        {"JapaneseFonts", SLOT_JAPANESEFONTS},
        {"Brightness", SLOT_BRIGHTNESS},
        {NULL, 0}};

void ScrSys_LoadPreferences()
{
    char **rows = Loader_LoadSTR(PreferencesFile);
    const char *par;
    int pos = 0;

    while (rows[pos] != NULL)
    {
        if (!str_empty(rows[pos]) && (par = strchr(rows[pos], '=')))
        {
            par = str_ltrim(par + 1);
            for (int j = 0; prefs[j].name != NULL; j++)
            {
                if (str_starts_with(rows[pos], prefs[j].name))
                {
                    LOG_INFO("'%s' = '%d'\n", prefs[j].name, atoi(par));
                    SetDirectgVarInt(prefs[j].slot, atoi(par));
                    break;
                }
            }
        }
        DELETE(rows[pos]);
        pos++;
    }

    DELETE(rows);
}

void ScrSys_SavePreferences()
{
    FILE *fl = fopen(PreferencesFile, "wb");
    if (!fl)
        return;

    fprintf(fl, "[%s]\r\n", GetGameTitle());

    for (int i = 0; prefs[i].name != NULL; i++)
        fprintf(fl, "%s=%d\r\n", prefs[i].name, GetgVarInt(prefs[i].slot));

    fclose(fl);
}
