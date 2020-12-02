#ifndef SCRIPTING_H_INCLUDED
#define SCRIPTING_H_INCLUDED

//Maximal number of same puzzles in the statebox stack
//For increasing speed of engine
//May cause errors, but should not
#define MaxPuzzlesInStack 2

#define PuzzleStack 2048

#define SLOT_WORLD 3
#define SLOT_ROOM 4
#define SLOT_NODE 5
#define SLOT_VIEW 6
#define SLOT_VIEW_POS 7
#define SLOT_KEY_PRESS 8       //keycode in slot
#define SLOT_INVENTORY_MOUSE 9 //code of item in slot
#define SLOT_MOUSE_DOWN 10     //1 if clicked
#define SLOT_ROUNDS 12
#define SLOT_VENUS 13
#define SLOT_MOUSE_RIGHT_CLICK 18 //1 if right button of mouse
#define SLOT_MENU_STATE 19
#define SLOT_JUST_RESTORED 20
#define SLOT_ON_QUIT 39 //1 - quitting
#define SLOT_LASTWORLD 40
#define SLOT_LASTROOM 41
#define SLOT_LASTNODE 42
#define SLOT_LASTVIEW 43
#define SLOT_LASTVIEW_POS 44
#define SLOT_MENU_LASTWORLD 45
#define SLOT_MENU_LASTROOM 46
#define SLOT_MENU_LASTNODE 47
#define SLOT_MENU_LASTVIEW 48
#define SLOT_MENU_LASTVIEW_POS 49
#define SLOT_KBD_ROTATE_SPEED 50
#define SLOT_SUBTITLE_FLAG 51 //ShowSubtitles
#define SLOT_STREAMSKIP_KEY 52
#define SLOT_PANAROTATE_SPEED 53
#define SLOT_MASTER_VOLUME 56
#define SLOT_QSOUND_ENABLE 57
#define SLOT_VENUSENABLED 58
#define SLOT_HIGH_QUIALITY 59
#define SLOT_LINE_SKIP_VIDEO 65
#define SLOT_PLATFORM 66
#define SLOT_INSTALL_LEVEL 67
#define SLOT_COUNTRY_CODE 68
#define SLOT_CPU 69
#define SLOT_MOVIE_CURSOR 70
#define SLOT_TURN_OFF_ANIM 71 //NoAnimWhileTurning
#define SLOT_WIN958 72
#define SLOT_SHOWERRORDIALOG 73
#define SLOT_DEBUGCHEATS 74 //if set to 1, we may type GOXXXX while game, and it will be changed location to XXXX
                            //To change - type DBGONOFF
#define SLOT_JAPANESEFONTS 75
#define SLOT_BRIGHTNESS 77
#define SLOT_EF9_B 91
#define SLOT_EF9_G 92
#define SLOT_EF9_R 93
#define SLOT_EF9_SPEED 94
#define SLOT_INV_STORAGE_S1 100
#define SLOT_INV_STORAGE_0 101
#define SLOT_INV_STORAGE_1 102
#define SLOT_INV_STORAGE_2 103
#define SLOT_INV_STORAGE_3 104
#define SLOT_INV_STORAGE_4 105
#define SLOT_INV_STORAGE_5 106
#define SLOT_INV_STORAGE_6 107
#define SLOT_INV_STORAGE_50 149
#define SLOT_TOTAL_INV_AVAIL 150

#define SLOT_START_SLOT 151
#define SLOT_END_SLOT 170

#define SLOT_SPELL_1 191
#define SLOT_SPELL_2 192
#define SLOT_SPELL_3 193
#define SLOT_SPELL_4 194
#define SLOT_SPELL_5 195
#define SLOT_SPELL_6 196
#define SLOT_SPELL_7 197
#define SLOT_SPELL_8 198
#define SLOT_SPELL_9 199
#define SLOT_SPELL_10 200
#define SLOT_SPELL_11 201
#define SLOT_SPELL_12 202
#define SLOT_USER_CHOSE_THIS_SPELL 205
#define SLOT_REVERSED_SPELLBOOK 206

#define FLAG_ONCE_PER_I 1
#define FLAG_DISABLED 2
#define FLAG_DO_ME_NOW 4

#define CRIT_OP_EQU 0 //=
#define CRIT_OP_LEA 1 //<
#define CRIT_OP_GRE 2 //>
#define CRIT_OP_NOT 3 //!

#define NODE_TYPE_MUSIC 0
#define NODE_TYPE_TIMER 1
#define NODE_TYPE_ANIMPLAY 2
#define NODE_TYPE_ANIMPRE 3
#define NODE_TYPE_ANIMPRPL 4
#define NODE_TYPE_PANTRACK 5
#define NODE_TYPE_TTYTEXT 6
#define NODE_TYPE_SYNCSND 7
#define NODE_TYPE_DISTORT 8
#define NODE_TYPE_REGION 9

#define NODE_RET_OK 0
#define NODE_RET_DELETE 1
#define NODE_RET_NO 2

typedef struct pzllst pzllst_t;

typedef struct
{
    uint16_t slot;   //puzzle slot
    MList CritList; //Criteria list of lists criteria
    MList ResList;  //results list
    pzllst_t *owner;
} puzzlenode_t;

typedef struct pzllst
{
    MList puzzles;
    puzzlenode_t *stack[PuzzleStack];
    size_t stksize;
    size_t exec_times;
} pzllst_t;

typedef struct struct_action_res
{
    int32_t slot;
    pzllst_t *owner;
    int8_t node_type;
    union nodes
    {
        musicnode_t *node_music;
        animnode_t *node_anim;
        animnode_t *node_animpre;
        anim_preplay_node_t *node_animpreplay;
        syncnode_t *node_sync;
        ttytext_t *tty_text;
        distort_t *distort;

        int32_t node_timer;
        int32_t node_pantracking;
        int32_t node_region;
        void *node_unknow;
    } nodes;
    bool need_delete;
    bool first_process;
} action_res_t;

pzllst_t *GetUni();
pzllst_t *Getworld();
pzllst_t *Getroom();
pzllst_t *Getview();
MList *GetControlsList();
MList *GetActionsList();

action_res_t *GetGNode(uint32_t indx);
void SetGNode(uint32_t indx, action_res_t *data);
void SetgVarInt(uint32_t indx, int var);
int GetgVarInt(uint32_t indx);
int *GetgVarRef(uint32_t indx);
void SetDirectgVarInt(uint32_t indx, int var);

void ScrSys_Init();
void ScrSys_LoadScript(pzllst_t *lst, const char *filename, bool control, MList *controlst);
void ScrSys_ChangeLocation(uint8_t w, uint8_t r, uint8_t v1, uint8_t v2, int32_t X, bool force_all);
uint8_t ScrSys_GetFlag(uint32_t indx);
void ScrSys_SetFlag(uint32_t indx, uint8_t newval);
void ScrSys_ExecPuzzleList(pzllst_t *lst);
const char *ScrSys_GetPuzzleListName(pzllst_t *lst);
void ScrSys_FlushResourcesByOwner(pzllst_t *owner);
void ScrSys_FlushResourcesByType(int type);
bool ScrSys_BreakExec();
void ScrSys_SetBreak();

void ScrSys_AddToActionsList(void *);
void ScrSys_ProcessActionsList();
void ScrSys_FlushActionsList();
int ScrSys_DeleteActionNode(action_res_t *nod);

void ScrSys_PrepareSaveBuffer();
void ScrSys_SaveGame(char *file);
void ScrSys_LoadGame(char *file);

void ScrSys_LoadPreferences();
void ScrSys_SavePreferences();

#endif // SCRIPTING_H_INCLUDED
