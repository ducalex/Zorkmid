#include "System.h"

static int32_t VAR_SLOTS_MAX = 20000;
static int32_t gVars[30000];
static action_res_t *gNodes[30000];
static uint8_t Flags[30000];
static StateBoxEnt_t *StateBox[30000];

static char PreferencesFile[128];

static bool BreakExecute = false;

static pzllst_t *uni = NULL;   //universe script
static pzllst_t *world = NULL; //world script
static pzllst_t *room = NULL;  //room script
static pzllst_t *view = NULL;  //view script

static MList *ctrl = NULL;   //contorls
static MList *actres = NULL; //sounds, animations, ttytexts and other.

static uint8_t *SaveBuffer = NULL;
static uint32_t SaveCurrentSize = 0;

void FillStateBoxFromList(pzllst_t *lst);
void ShakeStateBox(uint32_t indx);

static bool in_range(int indx)
{
    // const size_t max = sizeof(gVars) / sizeof(gVars[0]);
    return (indx >= 0 && indx < VAR_SLOTS_MAX); // > 0
}

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

MList *Getctrl()
{
    return ctrl;
}

MList *GetAction_res_List()
{
    return actres;
}

int tmr_DeleteTimer(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TIMER)
        return NODE_RET_NO;

    if (nod->nodes.node_timer < 0)
        SetgVarInt(nod->slot, 2);
    else
        SetgVarInt(nod->slot, nod->nodes.node_timer);

    setGNode(nod->slot, NULL);

    free(nod);

    return NODE_RET_DELETE;
}

int tmr_ProcessTimer(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TIMER)
        return NODE_RET_OK;

    if (nod->nodes.node_timer < 0)
    {
        tmr_DeleteTimer(nod);
        return NODE_RET_DELETE;
    }

    nod->nodes.node_timer -= GetDTime();

    return NODE_RET_OK;
}

void ScrSys_AddToActResList(void *nod)
{
    if (actres != NULL)
        AddToMList(actres, nod);
}

void SetgVarInt(int32_t indx, int var)
{
    if (in_range(indx))
    {
        gVars[indx] = var;
        ShakeStateBox(indx);
    }
}

void SetDirectgVarInt(uint32_t indx, int var)
{
    if (in_range(indx))
        gVars[indx] = var;
}

int GetgVarInt(int32_t indx)
{
    if (!in_range(indx))
        return 0;

    return gVars[indx];
}

int *GetDirectgVarInt(uint32_t indx)
{
    if (!in_range(indx))
        return NULL;

    return &gVars[indx];
}

uint8_t ScrSys_GetFlag(uint32_t indx)
{
    if (!in_range(indx))
        return 0;

    return Flags[indx];
}

void ScrSys_SetFlag(uint32_t indx, uint8_t newval)
{
    if (in_range(indx))
        Flags[indx] = newval;
}

action_res_t *getGNode(int32_t indx)
{
    if (!in_range(indx))
        return NULL;

    return gNodes[indx];
}

void setGNode(int32_t indx, action_res_t *data)
{
    if (in_range(indx))
        gNodes[indx] = data;
}

void InitScriptsEngine()
{
    SaveBuffer = (uint8_t *)malloc(512 * 1024);

    if (CUR_GAME == GAME_ZGI)
    {
        VAR_SLOTS_MAX = 20000;
        strcpy(PreferencesFile, "prefs_zgi.ini");
    }
    else
    {
        VAR_SLOTS_MAX = 30000;
        strcpy(PreferencesFile, "prefs_znem.ini");
    }

    memset(StateBox, 0x0, sizeof(StateBox));
    memset(Flags, 0x0, sizeof(Flags));
    memset(gVars, 0x0, sizeof(gVars));
    memset(gNodes, 0x0, sizeof(gNodes));

    view = CreatePuzzleList("view");
    room = CreatePuzzleList("room");
    world = CreatePuzzleList("world");
    uni = CreatePuzzleList("universe");
    ctrl = CreateMList();
    actres = CreateMList();

    //needed for znemesis
    SetDirectgVarInt(SLOT_CPU, 1);
    SetDirectgVarInt(SLOT_PLATFORM, 0);
    SetDirectgVarInt(SLOT_WIN958, 0);

    ScrSys_LoadPreferences();
}

void LoadScriptFile(pzllst_t *lst, FManNode_t *filename, bool control, MList *controlst)
{
#ifdef TRACE
    printf("Loading script file %s\n", filename->File);
#endif

    if (control)
    {
        Rend_SetRenderer(RENDER_FLAT);
    }

    mfile_t *fl = mfopen(filename);
    if (fl == NULL)
    {
        Z_PANIC("Error opening file %s\n", filename->File);
        return;
    }

    char buf[STRBUFSIZE];

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);

        char *str = PrepareString(buf);

        if (strCMP(str, "puzzle") == 0)
        {
            Puzzle_Parse(lst, fl, str);
        }
        else if (strCMP(str, "control") == 0 && control)
        {
            Parse_Control(controlst, fl, str);
        }
    }

    mfclose(fl);
}

void ScrSys_ClearStateBox()
{
    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        if (StateBox[i] != NULL)
            free(StateBox[i]);
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

    MList *lst = GetAction_res_List();
    StartMList(lst);
    while (!eofMList(lst))
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

    lst = GetAction_res_List();
    StartMList(lst);
    while (!eofMList(lst))
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

    FILE *f = fopen(file, "rb");
    if (!f)
        return;

    ScrSys_FlushActResList();

    fread(&tmp, 4, 1, f);
    if (tmp != 0x47534E5A)
    {
        Z_PANIC("Error in save file %s\n", file);
    }

    fread(&tmp, 4, 1, f);
    fread(&tmp, 4, 1, f);
    fread(&tmp, 4, 1, f);
    fread(&tmp, 4, 1, f);

    fread(&w, 1, 1, f);
    fread(&r, 1, 1, f);
    fread(&n, 1, 1, f);
    fread(&v, 1, 1, f);

    fread(&pos, 2, 1, f);

    fread(&tmp, 2, 1, f);

    while (!feof(f))
    {
        fread(&tmp, 4, 1, f);
        if (tmp != 0x524D4954)
            break;

        fread(&tmp, 4, 1, f);

        fread(&slot, 4, 1, f);
        fread(&time, 4, 1, f);

        sprintf(buf, "%d", time / 100);
        action_timer(buf, slot, view);
    }

    fread(&tmp, 4, 1, f);

    if (tmp != VAR_SLOTS_MAX * 2)
    {
        Z_PANIC("Error in save file %s (FLAGS VAR_SLOTS_MAX)\n", file);
    }
    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        fread(&tmp2, 2, 1, f);
        ScrSys_SetFlag(i, tmp2);
    }

    fread(&tmp, 4, 1, f);
    fread(&tmp, 4, 1, f);
    if (tmp != VAR_SLOTS_MAX * 2)
    {
        Z_PANIC("Error in save file %s (PUZZLE VAR_SLOTS_MAX)\n", file);
    }
    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        fread(&tmp2, 2, 1, f);
        SetDirectgVarInt(i, tmp2);
    }

    Rend_SetDelay(2);

    ScrSys_ChangeLocation(w, r, n, v, pos, true);

    SetgVarInt(SLOT_JUST_RESTORED, 1);

    fclose(f);

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

        FlushPuzzleList(view);
        FlushControlList(ctrl);

        tm[0] = temp.World;
        tm[1] = temp.Room;
        tm[2] = temp.Node;
        tm[3] = temp.View;
        tm[4] = 0;
        sprintf(buf, "%s.scr", tm);
        FManNode_t *fil = FindInBinTree(buf);
        if (fil != NULL)
            LoadScriptFile(view, fil, true, ctrl);
    }

    if (temp.Room != GetgVarInt(SLOT_ROOM) ||
        temp.World != GetgVarInt(SLOT_WORLD) ||
        force_all || room == NULL)
    {
        ScrSys_FlushResourcesByOwner(room);

        FlushPuzzleList(room);

        tm[0] = temp.World;
        tm[1] = temp.Room;
        tm[2] = 0;
        sprintf(buf, "%s.scr", tm);

        FManNode_t *fil = FindInBinTree(buf);
        if (fil != NULL)
            LoadScriptFile(room, fil, false, NULL);
    }

    if (temp.World != GetgVarInt(SLOT_WORLD) ||
        force_all || world == NULL)
    {
        ScrSys_FlushResourcesByOwner(world);

        FlushPuzzleList(world);

        tm[0] = temp.World;
        tm[1] = 0;
        sprintf(buf, "%s.scr", tm);

        FManNode_t *fil = FindInBinTree(buf);
        if (fil != NULL)
            LoadScriptFile(world, fil, false, NULL);

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

    menu_SetMenuBarVal(0xFFFF);

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
    while (!eofMList(lst->_list))
    {
        puzzlenode_t *pzlnod = (puzzlenode_t *)DataMList(lst->_list);

        AddPuzzleToStateBox(pzlnod->slot, pzlnod);

        StartMList(pzlnod->CritList);
        while (!eofMList(pzlnod->CritList))
        {
            MList *CriteriaLst = (MList *)DataMList(pzlnod->CritList);

            int prevslot = 0;
            StartMList(CriteriaLst);
            while (!eofMList(CriteriaLst))
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
#ifdef TRACE
        printf("Can't add pzl# %d to Stack\n", pzl->slot);
#endif
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
        while (!eofMList(lst->_list))
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

void ScrSys_ProcessActResList()
{
    MList *lst = GetAction_res_List();

    int result = NODE_RET_OK;

    StartMList(lst);
    while (!eofMList(lst))
    {
        action_res_t *nod = (action_res_t *)DataMList(lst);

        nod->first_process = true;

        if (!nod->need_delete)
        {
            switch (nod->node_type)
            {
            case NODE_TYPE_MUSIC:
                result = snd_ProcessWav(nod);
                break;
            case NODE_TYPE_TIMER:
                result = tmr_ProcessTimer(nod);
                break;
            case NODE_TYPE_ANIMPLAY:
                result = anim_ProcessAnimPlayNode(nod);
                break;
            case NODE_TYPE_ANIMPRE:
                result = anim_ProcessAnimPreNode(nod);
                break;
            case NODE_TYPE_ANIMPRPL:
                result = anim_ProcessAnimPrePlayNode(nod);
                break;
            case NODE_TYPE_SYNCSND:
                result = snd_ProcessSync(nod);
                break;
            case NODE_TYPE_PANTRACK:
                result = snd_ProcessPanTrack(nod);
                break;
            case NODE_TYPE_TTYTEXT:
                result = txt_ProcessTTYtext(nod);
                break;
            case NODE_TYPE_DISTORT:
                result = Rend_ProcessDistortNode(nod);
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
            ScrSys_DeleteNode(nod);
        }

        if (result == NODE_RET_DELETE)
            DeleteCurrent(lst);

        NextMList(lst);
    }
}

int ScrSys_DeleteNode(action_res_t *nod)
{
    switch (nod->node_type)
    {
    case NODE_TYPE_MUSIC:
        return snd_DeleteWav(nod);
    case NODE_TYPE_TIMER:
        return tmr_DeleteTimer(nod);
    case NODE_TYPE_ANIMPLAY:
        return anim_DeleteAnimPlay(nod);
    case NODE_TYPE_ANIMPRE:
        return anim_DeleteAnimPreNod(nod);
    case NODE_TYPE_ANIMPRPL:
        return anim_DeleteAnimPrePlayNode(nod);
    case NODE_TYPE_SYNCSND:
        return snd_DeleteSync(nod);
    case NODE_TYPE_PANTRACK:
        return snd_DeletePanTrack(nod);
    case NODE_TYPE_TTYTEXT:
        return txt_DeleteTTYtext(nod);
    case NODE_TYPE_DISTORT:
        return Rend_DeleteDistortNode(nod);
    case NODE_TYPE_REGION:
        return Rend_DeleteRegion(nod);
    default:
        return NODE_RET_NO;
    }
}

void ScrSys_FlushActResList()
{
    MList *all = GetAction_res_List();

    StartMList(all);
    while (!eofMList(all))
    {
        ScrSys_DeleteNode((action_res_t *)DataMList(all));
        NextMList(all);
    }
    FlushMList(all);
}

void ScrSys_FlushResourcesByOwner(pzllst_t *owner)
{
    MList *all = GetAction_res_List();

    StartMList(all);
    while (!eofMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->owner == owner)
        {
            int result = NODE_RET_OK;

            if (nod->node_type == NODE_TYPE_MUSIC)
            {
                if (nod->nodes.node_music->universe == false)
                    result = snd_DeleteWav(nod);
            }
            else
                result = ScrSys_DeleteNode(nod);

            if (result == NODE_RET_DELETE)
                DeleteCurrent(all);
        }

        NextMList(all);
    }
}

void ScrSys_FlushResourcesByType(int type)
{
    MList *all = GetAction_res_List();

    StartMList(all);
    while (!eofMList(all))
    {
        action_res_t *nod = (action_res_t *)DataMList(all);

        if (nod->node_type == type && nod->first_process == true)
            if (ScrSys_DeleteNode(nod) == NODE_RET_DELETE)
                DeleteCurrent(all);

        NextMList(all);
    }
}

action_res_t *ScrSys_CreateActRes(int type)
{
    action_res_t *tmp = NEW(action_res_t);
    tmp->node_type = type;
    return tmp;
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
    FILE *fl = fopen(PreferencesFile, "rb");
    if (!fl)
        return;

    char buffer[128];
    char *str;

    while (!feof(fl))
    {
        fgets(buffer, 128, fl);
        str = TrimLeft(TrimRight(buffer));
        if (str != NULL && strlen(str) > 0)
            for (int i = 0; prefs[i].name != NULL; i++)
                if (strCMP(str, prefs[i].name) == 0)
                {
                    str = strstr(str, "=");
                    if (str != NULL)
                    {
                        str++;
                        str = TrimLeft(str);
                        SetDirectgVarInt(prefs[i].slot, atoi(str));
                    }
                    break;
                }
    }

    fclose(fl);
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
