#include "Utilities.h"
#include "Scripting.h"
#include "Controls.h"
#include "Actions.h"
#include "Render.h"
#include "Loader.h"
#include "Anims.h"
#include "Timer.h"
#include "Menu.h"
#include "Game.h"

typedef struct
{
    dynlist_t puzzles;
    dynlist_t stack;
    size_t exec_times;
} pzllst_t;

typedef struct
{
    int slot1;
    int slot2;
    char oper;
    bool var2; //if true: slot2 is slot; false: slot2 - number
} crit_node_t;

typedef struct
{
    char action[32];
    char params[128];
    int slot;
} res_node_t;

static int32_t VAR_SLOTS_MAX = 20000;
static int32_t gVars[30000];
static action_res_t *gNodes[30000];
static uint8_t Flags[30000];
static dynlist_t StateBox[30000]; // Lists of puzzlenode_t
static pzllst_t PuzzleLists[LIST_COUNT]; // universe, world, room, view
static dynlist_t controls;  // Controls
static dynlist_t actions;   // Sounds, animations, ttytexts, and others

static bool BreakExecute = false;

static const char *PreferencesFile = "prefs.ini";

static uint8_t SaveBuffer[512 * 1024];
static size_t SaveCurrentSize = 0;


static void DeletePuzzleNode(puzzlenode_t *nod)
{
    LOG_DEBUG("Deleting Puzzle #%d\n", nod->slot);

    for (int i = 0; i < nod->CritList.length; i++)
    {
        dynlist_t *critlist = (dynlist_t *)nod->CritList.items[i];
        if (!critlist) continue;

        for (int j = 0; j < critlist->length; j++)
        {
            DELETE(critlist->items[j]);
        }
        DeleteList(critlist);
    }
    DeleteList(&nod->CritList);

    for (int i = 0; i < nod->ResList.length; i++)
    {
        DELETE(nod->ResList.items[i]);
    }
    DeleteList(&nod->ResList);

    DELETE(nod);
}

static void FlushPuzzleList(zlist_t index)
{
    pzllst_t *lst = &PuzzleLists[index];

    for (int i = 0; i < lst->puzzles.length; i++)
    {
        if (lst->puzzles.items[i])
            DeletePuzzleNode(lst->puzzles.items[i]);
    }

    FlushList(&lst->puzzles);
    FlushList(&lst->stack);

    lst->exec_times = 0;
}

static void ParsePuzzleFlags(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            return;
        }
        else if (str_starts_with(str, "once_per_inst"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_ONCE_PER_I);
        }
        else if (str_starts_with(str, "do_me_now"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_DO_ME_NOW);
        }
        else if (str_starts_with(str, "disabled"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_DISABLED);
        }
    }
}

static void ParsePuzzleCriteria(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;
    int left, right;
    char op;

    dynlist_t *crit_nodes_lst = CreateList(64);

    AddToList(&pzl->CritList, crit_nodes_lst);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            return;
        }
        else if (sscanf(str, "[%d] %c [%d]", &left, &op, &right) == 3)
        {
            crit_node_t *nod = NEW(crit_node_t);
            AddToList(crit_nodes_lst, nod);
            nod->slot1 = left;
            nod->oper = op;
            nod->slot2 = right;
            nod->var2 = true;
        }
        else if (sscanf(str, "[%d] %c %d", &left, &op, &right) == 3)
        {
            crit_node_t *nod = NEW(crit_node_t);
            AddToList(crit_nodes_lst, nod);
            nod->slot1 = left;
            nod->oper = op;
            nod->slot2 = right;
            nod->var2 = false;
        }
        else
        {
            LOG_WARN("Criteria parsing error in: '%s'\n", str);
        }
    }
}

static void ParsePuzzleResultAction(char *str, dynlist_t *list)
{
    char buf[255];
    const char *params = " ";
    int slot = 0;
    int len = strlen(str);

    memset(buf, 0, 255);

    // Format is: action:slot(param1 param2 param3)
    // Where param count is variable and slot not always present

    for (int i = 0; i < len; i++)
    {
        if (str[i] != '(' && str[i] != 0x20 && str[i] != 0x09 && str[i] != '#' && str[i] != ':')
        {
            buf[i] = str[i];
        }
        else
        {
            if (str[i] == ':')
                slot = atoi(str + i + 1);
            params = GetParams(str + i);
            break;
        }
    }

    res_node_t *nod = NEW(res_node_t);
    AddToList(list, nod);
    strcpy(nod->action, buf);
    strcpy(nod->params, params);
    nod->slot = slot;
}

static void ParsePuzzleResults(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;

    while (!mfeof(fl))
    {
        // Format is `action: background: event: other`
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
            return;
        else if ((str = strchr(str, ':')))
            ParsePuzzleResultAction(str + 1, &pzl->ResList);
        else
            LOG_WARN("Results parsing error in: '%s'\n", str);
    }
}

static void ParsePuzzle(zlist_t owner, mfile_t *fl, char *ctstr)
{
    char buf[STRBUFSIZE];
    char *str;
    int slot;

    sscanf(ctstr, "puzzle:%d", &slot);

    puzzlenode_t *pzl = NEW(puzzlenode_t);
    pzl->owner = owner;
    pzl->slot = slot;

    ScrSys_SetFlag(pzl->slot, 0);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}') // We've reached the end!
        {
            pzllst_t *list = &PuzzleLists[owner];

            if ((ScrSys_GetFlag(pzl->slot) & FLAG_ONCE_PER_I))
                SetgVarInt(slot, 0);

            LOG_DEBUG("Created Puzzle %d\n", slot);
            AddToList(&list->puzzles, pzl);
            return;
        }
        else if (str_starts_with(str, "criteria"))
        {
            ParsePuzzleCriteria(pzl, fl);
        }
        else if (str_starts_with(str, "results"))
        {
            ParsePuzzleResults(pzl, fl);
        }
        else if (str_starts_with(str, "flags"))
        {
            ParsePuzzleFlags(pzl, fl);
        }
    }

    // If we reach that point the parsing failed...
    LOG_WARN("Failed to parse puzzle '%s'...", ctstr);
    DeletePuzzleNode(pzl);
}

static bool ProcessCriteries(dynlist_t *lst)
{
    bool tmp = true;

    for (int i = 0; i < lst->length; i++)
    {
        crit_node_t *critnd = (crit_node_t *)lst->items[i];
        if (!critnd) continue;

        LOG_DEBUG("  [%d] %d [%d] %d\n", critnd->slot1, critnd->oper, critnd->slot2, critnd->var2);

        int tmp1 = GetgVarInt(critnd->slot1);
        int tmp2 = critnd->var2 ? GetgVarInt(critnd->slot2) : critnd->slot2;

        switch (critnd->oper)
        {
        case '=':
            tmp &= (tmp1 == tmp2);
            break;
        case '>':
            tmp &= (tmp1 > tmp2);
            break;
        case '<':
            tmp &= (tmp1 < tmp2);
            break;
        case '!':
            tmp &= (tmp1 != tmp2);
            break;
        }

        if (!tmp)
            break;
    }

    return tmp;
}

static int ExecPuzzle(puzzlenode_t *pzlnod)
{
    if (ScrSys_GetFlag(pzlnod->slot) & FLAG_DISABLED)
        return ACTION_NORMAL;

    if (GetgVarInt(pzlnod->slot) == 1)
        return ACTION_NORMAL;

    if (PuzzleLists[pzlnod->owner].exec_times == 0)
        if (!(ScrSys_GetFlag(pzlnod->slot) & FLAG_DO_ME_NOW))
            return ACTION_NORMAL;

    if (pzlnod->CritList.length > 0)
    {
        int match = -1;

        for (int i = 0; i < pzlnod->CritList.length; i++)
        {
            dynlist_t *crits = (dynlist_t *)pzlnod->CritList.items[i];
            if (!crits) continue;

            if ((match = ProcessCriteries(crits)))
                break;
        }

        // We iterated at least once but got no match
        if (match == 0)
            return ACTION_NORMAL;
    }

    LOG_DEBUG("Running puzzle %d (owner: %d)\n", pzlnod->slot, pzlnod->owner);

    SetgVarInt(pzlnod->slot, 1);

    for (int i = 0; i < pzlnod->ResList.length; i++)
    {
        res_node_t *res = (res_node_t *)pzlnod->ResList.items[i];
        if (!res) continue;

        if (Actions_Run(res->action, res->params, res->slot, pzlnod->owner) == ACTION_BREAK)
        {
            return ACTION_BREAK;
        }
    }

    return ACTION_NORMAL;
}

static void FillStateBoxFromList(zlist_t index)
{
    pzllst_t *lst = &PuzzleLists[index];

    for (int i = 0; i < lst->puzzles.length; i++)
    {
        puzzlenode_t *pzlnod = (puzzlenode_t *)lst->puzzles.items[i];
        if (!pzlnod) continue;

        AddToList(&StateBox[pzlnod->slot], pzlnod);

        for (int j = 0; j < pzlnod->CritList.length; j++)
        {
            dynlist_t *CriteriaLst = (dynlist_t *)pzlnod->CritList.items[j];
            if (!CriteriaLst) continue;

            int prevslot = 0;
            for (int k = 0; k < CriteriaLst->length; k++)
            {
                crit_node_t *crtnod = (crit_node_t *)CriteriaLst->items[k];
                if (!crtnod) continue;

                if (prevslot != crtnod->slot1)
                    AddToList(&StateBox[crtnod->slot1], pzlnod);

                prevslot = crtnod->slot1;
            }
        }
    }
}

static void AddStateBoxToStk(puzzlenode_t *pzl)
{
    pzllst_t *owner = &PuzzleLists[pzl->owner];

    if (owner->stack.length > 0)
    {
        int numb = 0;
        for (int i = owner->stack.length - 1; i >= 0; i--)
        {
            if (owner->stack.items[i] == pzl)
            {
                if (numb < MaxPuzzlesInStack)
                    numb++;
                else
                    return;
            }
        }
    }

    AddToList(&owner->stack, pzl);
}

static void ShakeStateBox(uint32_t indx)
{
    if (CURRENT_GAME == GAME_ZNEM)
    {
        for (int i = 0; i < StateBox[indx].length; i++)
            AddStateBoxToStk(StateBox[indx].items[i]);
    }
    else
    {
        for (int i = StateBox[indx].length - 1; i >= 0; i--)
            AddStateBoxToStk(StateBox[indx].items[i]);
    }
}

static void FlushStateBox()
{
    for (int i = 0; i < VAR_SLOTS_MAX; i++)
    {
        for (int j = 0; j < StateBox[i].length; j++)
        {
            //  DeletePuzzleNode(StateBox[i].items[j]); // Should we ?
        }
        FlushList(&StateBox[i]);
    }
}

dynlist_t *GetControlsList()
{
    return &controls;
}

dynlist_t *GetActionsList()
{
    return &actions;
}

void ScrSys_AddToActionsList(void *nod)
{
    AddToList(&actions, nod);
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
    if (CURRENT_GAME == GAME_ZGI)
    {
        PreferencesFile = "./prefs_zgi.ini";
        VAR_SLOTS_MAX = 20000;
    }
    else
    {
        PreferencesFile = "./prefs_znem.ini";
        VAR_SLOTS_MAX = 30000;
    }

    //needed for znemesis
    SetDirectgVarInt(SLOT_CPU, 1);
    SetDirectgVarInt(SLOT_PLATFORM, 0);
    SetDirectgVarInt(SLOT_WIN958, 0);

    ScrSys_LoadPreferences();
}

void ScrSys_LoadScript(zlist_t list, const char *filename, bool control, dynlist_t *controls)
{
    mfile_t *fl = mfopen(filename);
    if (!fl)
        Z_PANIC("Unable to load script file '%s'\n", filename);

    LOG_DEBUG("Loading script file '%s'\n", filename);

    if (control)
        Rend_SetRenderer(RENDER_FLAT);

    char buf[STRBUFSIZE];
    char *str;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str_starts_with(str, "puzzle"))
        {
            ParsePuzzle(list, fl, str);
        }
        else if (str_starts_with(str, "control") && control)
        {
            Control_Parse(controls, fl, str);
        }
    }

    mfclose(fl);
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

    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

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

    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

        if (nod->slot > 0 && nod->node_type == NODE_TYPE_MUSIC)
            tmp2[nod->slot] = 2;
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
        Actions_Run("timer", buf, slot, LIST_VIEW);
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

void ScrSys_ChangeLocation(uint8_t w, uint8_t r, uint8_t n, uint8_t v, int32_t X, bool force_all)
{
    char buf[32];

    if (GetgVarInt(SLOT_WORLD) != SystemWorld || GetgVarInt(SLOT_ROOM) != SystemRoom)
    {
        if (w == SystemWorld && r == SystemRoom)
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

    FlushStateBox();

    if (w == SaveWorld && r == SaveRoom && n == SaveNode && v == SaveView)
    {
        ScrSys_PrepareSaveBuffer();
    }

    if (v != GetgVarInt(SLOT_VIEW) || n != GetgVarInt(SLOT_NODE) ||
        r != GetgVarInt(SLOT_ROOM) || w != GetgVarInt(SLOT_WORLD) ||
        force_all)
    {
        ScrSys_FlushResourcesByOwner(LIST_VIEW);
        FlushPuzzleList(LIST_VIEW);
        Controls_FlushList(&controls);
        sprintf(buf, "%c%c%c%c.scr", w, r, n, v);
        ScrSys_LoadScript(LIST_VIEW, buf, true, &controls);
    }

    if (r != GetgVarInt(SLOT_ROOM) || w != GetgVarInt(SLOT_WORLD) || force_all)
    {
        ScrSys_FlushResourcesByOwner(LIST_ROOM);
        FlushPuzzleList(LIST_ROOM);
        sprintf(buf, "%c%c.scr", w, r);
        ScrSys_LoadScript(LIST_ROOM, buf, false, NULL);
    }

    if (w != GetgVarInt(SLOT_WORLD) || force_all)
    {
        ScrSys_FlushResourcesByOwner(LIST_WORLD);
        FlushPuzzleList(LIST_WORLD);
        sprintf(buf, "%c.scr", w);
        ScrSys_LoadScript(LIST_WORLD, buf, false, NULL);
        Mouse_ShowCursor();
    }

    FillStateBoxFromList(LIST_UNIVERSE);
    FillStateBoxFromList(LIST_VIEW);
    FillStateBoxFromList(LIST_ROOM);
    FillStateBoxFromList(LIST_WORLD);

    SetgVarInt(SLOT_WORLD, w);
    SetgVarInt(SLOT_ROOM, r);
    SetgVarInt(SLOT_NODE, n);
    SetgVarInt(SLOT_VIEW, v);
    SetgVarInt(SLOT_VIEW_POS, X);

    Menu_SetVal(0xFFFF);

    BreakExecute = false;
}

void ScrSys_ExecPuzzleList(zlist_t index)
{
    pzllst_t *list = &PuzzleLists[index];
    if (list->exec_times < 2)
    {
        for (int i = 0; i < list->puzzles.length; i++)
        {
            puzzlenode_t *nod = (puzzlenode_t *)list->puzzles.items[i];
            if (!nod) continue;

            if (ExecPuzzle(nod) == ACTION_BREAK)
            {
                BreakExecute = true;
                break;
            }
        }
        list->exec_times++;
    }
    else
    {
        int i = 0, j = list->stack.length;

        while (i < j)
        {
            puzzlenode_t *to_exec = list->stack.items[i];

            DeleteFromList(&list->stack, i);

            if (ExecPuzzle(to_exec) == ACTION_BREAK)
            {
                BreakExecute = true;
                break;
            }

            i++;
        }

        int z = 0;
        for (i = j; i < list->stack.length; i++)
        {
            list->stack.items[z] = list->stack.items[i];
            z++;
        }
        list->stack.length = z;
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
    int result = NODE_RET_OK;

    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

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
            DeleteFromList(&actions, i);
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
    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

        ScrSys_DeleteActionNode(nod);
    }
    FlushList(&actions);
}

void ScrSys_FlushResourcesByOwner(zlist_t owner)
{
    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

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
                actions.items[i] = NULL;
        }
    }
}

void ScrSys_FlushResourcesByType(int type)
{
    for (int i = 0; i < actions.length; i++)
    {
        action_res_t *nod = (action_res_t *)actions.items[i];
        if (!nod) continue;

        if (nod->node_type == type && nod->first_process == true)
            if (ScrSys_DeleteActionNode(nod) == NODE_RET_DELETE)
                actions.items[i] = NULL;
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
    char line[STRBUFSIZE];
    char key[MINIBUFSIZE];
    int value;

    mfile_t *mfp = mfopen_txt(PreferencesFile);
    if (!mfp)
        return;

    while (!mfeof(mfp))
    {
        mfgets(line, STRBUFSIZE, mfp);

        if (sscanf(line, "%[^=]=%d", key, &value) == 2)
        {
            for (int j = 0; prefs[j].name != NULL; j++)
            {
                if (str_equals(key, prefs[j].name))
                {
                    LOG_DEBUG("'%s' = '%d'\n", prefs[j].name, value);
                    SetDirectgVarInt(prefs[j].slot, value);
                    break;
                }
            }
        }
    }

    mfclose(mfp);
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
