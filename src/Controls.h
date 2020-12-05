#ifndef CONTROL_H_INCLUDED
#define CONTROL_H_INCLUDED

#include "Loader.h"

#define CTRL_FIST_MAX_FISTS 4
#define CTRL_FIST_MAX_BOXES 6
#define CTRL_FIST_MAX_ENTRS 64

#define CTRL_TITLER_MAX_STRINGS 128

#define CTRL_LEVER_MAX_FRAMES 128
#define CTRL_LEVER_MAX_DIRECTS 4
#define CTRL_LEVER_ANGL_TIME 50
#define CTRL_LEVER_ANGL_DELTA 30
#define CTRL_LEVER_AUTO_DELAY FPS_DELAY

#define MAX_SAVES 100
#define SAVE_NAME_MAX_LEN 20

#define CTRL_UNKN 0
#define CTRL_PUSH 1
#define CTRL_INPUT 2
#define CTRL_SLOT 3
#define CTRL_SAVE 4
#define CTRL_LEVER 5
#define CTRL_SAFE 6
#define CTRL_FIST 7
#define CTRL_HOTMV 8
#define CTRL_PAINT 9
#define CTRL_TITLER 10
#define CTRL_PANA 100
#define CTRL_FLAT 101
#define CTRL_TILT 102

#define CTRL_PUSH_EV_UP 0
#define CTRL_PUSH_EV_DWN 1
#define CTRL_PUSH_EV_DBL 2

#define CTRL_SAVE_FILE (CURRENT_GAME == GAME_ZGI ? "./inquis.sav" : "./nemesis.sav")
#define CTRL_SAVE_SAVES (CURRENT_GAME == GAME_ZGI ? "./inqsav%d.sav" : "./nemsav%d.sav")

typedef struct ctrlnode ctrlnode_t;

typedef struct
{
    int x;
    int y;
    union { int w; int x2; };
    union { int h; int y2; };
} Rect_t;

typedef struct
{
    bool flat; //true - flat, false - warp
    int x;
    int y;
    int w;
    int h;
    int count_to;
    int cursor;
    int event; // 0 - up, 1 - down, 2 - double
} pushnode_t;

typedef struct
{
    bool flat; //true - flat, false - warp
    Rect_t rectangle;
    char distance_id[MINIBUFSIZE];
    Rect_t hotspot;
    int *eligible_objects;
    int eligable_cnt;
    int cursor;
    int loaded_img;
    SDL_Surface *srf;
} slotnode_t;

typedef struct
{
    Rect_t rectangle;
    Rect_t hotspot;
    SDL_Surface *rect;
    int next_tab;
    char text[SAVE_NAME_MAX_LEN + 1];
    int textwidth;
    bool textchanged;
    anim_surf_t *cursor;
    int frame;
    int period;
    bool readonly;
    bool enterkey;
    bool focused;
    txt_style_t string_init;
    txt_style_t string_chooser_init;
} inputnode_t;

typedef struct
{
    bool forsaving;
    int inputslot[MAX_SAVES];
    ctrlnode_t *input_nodes[MAX_SAVES];
    char Names[MAX_SAVES][SAVE_NAME_MAX_LEN + 1];
} saveloadnode_t;

typedef struct
{
    int cursor;
    //int animation_id;
    bool mirrored;
    //int skipcolor;
    Rect_t AnimCoords;
    int frames;
    int startpos;
    animnode_t *anm;
    int curfrm;
    int rendfrm;
    struct hotspots
    {
        int x;
        int y;
        int angles;
        struct directions
        {
            int toframe;
            int angle;
        } directions[CTRL_LEVER_MAX_DIRECTS];
    } hotspots[CTRL_LEVER_MAX_FRAMES];
    int delta_x;
    int delta_y;
    int last_mouse_x;
    int last_mouse_y;
    int mouse_angle;
    int mouse_count;
    bool mouse_captured;
    int hasout[CTRL_LEVER_MAX_FRAMES];                         //seq's frame count
    int outproc[CTRL_LEVER_MAX_FRAMES][CTRL_LEVER_MAX_FRAMES]; //seq's for every frames
    bool autoout;                                                  //seq initiated
    int autoseq;                                               //what frame initiate this seq
    int autoseq_frm;                                           //current seq frame
    int autoseq_time;                                          //time leave to next seq frame
} levernode_t;

typedef struct
{
    int num_states;
    int cur_state;
    animnode_t *anm;
    int center_x;
    int center_y;
    Rect_t rectangle;
    int radius_inner;
    int radius_inner_sq;
    int radius_outer;
    int radius_outer_sq;
    int zero_pointer;
    int start_pointer;
    int cur_frame;
    int to_frame;
    int frame_time;
} safenode_t;

typedef struct
{
    int num_strings;
    int current_string;
    int next_string;
    char *strings[CTRL_TITLER_MAX_STRINGS];
    Rect_t rectangle;
    SDL_Surface *surface;
} titlernode_t;

typedef struct
{
    uint8_t *brush;
    int b_w;
    int b_h;
    int last_x;
    int last_y;
    int *eligible_objects;
    int eligable_cnt;
    int cursor;
    SDL_Surface *paint;
    Rect_t rectangle;
} paintnode_t;

typedef struct
{
    int fiststatus;
    int fistnum;
    int cursor;
    int order;
    struct fists_up
    {
        int num_box;
        Rect_t boxes[CTRL_FIST_MAX_BOXES];
    } fists_up[CTRL_FIST_MAX_FISTS];
    struct fists_dwn
    {
        int num_box;
        Rect_t boxes[CTRL_FIST_MAX_BOXES];
    } fists_dwn[CTRL_FIST_MAX_FISTS];

    int num_entries;
    struct entries
    {
        int strt;
        int send;
        int anm_str;
        int anm_end;
        int sound;
    } entries[CTRL_FIST_MAX_ENTRS];

    animnode_t *anm;
    Rect_t anm_rect;
    int soundkey;
    int frame_cur;
    int frame_end;
    int frame_time;
    int animation_id;
} fistnode_t;

typedef struct
{
    int num_frames;
    int frame_time;
    int cur_frame;
    int rend_frame;
    int cycle;
    int num_cycles;
    Rect_t *frame_list;
    animnode_t *anm;
    Rect_t rect;
} hotmvnode_t;

typedef struct ctrlnode
{
    int slot;
    int type;
    int venus;
    union node
    {
        slotnode_t *slot;
        pushnode_t *push;
        inputnode_t *inp;
        saveloadnode_t *svld;
        levernode_t *lev;
        safenode_t *safe;
        fistnode_t *fist;
        hotmvnode_t *hotmv;
        paintnode_t *paint;
        titlernode_t *titler;

        void *unknown;
    } node;
    void (*func)(ctrlnode_t *);
} ctrlnode_t;

void Control_Parse(dynlist_t *controls, mfile_t *fl, char *ctstr);
void Controls_ProcessList(dynlist_t *list);
void Controls_Draw();
void Controls_FlushList(dynlist_t *list);
ctrlnode_t *Controls_GetControl(int id);

#endif // CONTROL_H_INCLUDED
