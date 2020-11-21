#ifndef CONTROL_H_INCLUDED
#define CONTROL_H_INCLUDED

#include "System.h"

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

typedef struct
{
    int32_t x;
    int32_t y;
    union
    {
        int32_t w;
        int32_t x2;
    };
    union
    {
        int32_t h;
        int32_t y2;
    };
} Rect_t;

typedef struct
{
    bool flat; //true - flat, false - warp
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    int16_t count_to;
    int16_t cursor;
    int8_t event; // 0 - up, 1 - down, 2 - double
} pushnode_t;

typedef struct
{
    bool flat; //true - flat, false - warp
    Rect_t rectangle;
    char distance_id[MINIBUFSZ];
    Rect_t hotspot;
    int32_t *eligible_objects;
    int32_t eligable_cnt;
    int16_t cursor;
    int32_t loaded_img;
    SDL_Surface *srf;
} slotnode_t;

typedef struct
{
    Rect_t rectangle;
    Rect_t hotspot;
    SDL_Surface *rect;
    int32_t next_tab;
    char text[SAVE_NAME_MAX_LEN + 1];
    int16_t textwidth;
    bool textchanged;
    anim_surf_t *cursor;
    int32_t frame;
    int32_t period;
    bool readonly;
    bool enterkey;
    bool focused;
    txt_style_t string_init;
    txt_style_t string_chooser_init;
} inputnode_t;

typedef struct
{
    bool forsaving;
    int32_t inputslot[MAX_SAVES];
    ctrlnode_t *input_nodes[MAX_SAVES];
    char Names[MAX_SAVES][SAVE_NAME_MAX_LEN + 1];
} saveloadnode_t;

typedef struct
{
    int16_t cursor;
    //int32_t animation_id;
    bool mirrored;
    //uint32_t skipcolor;
    Rect_t AnimCoords;
    int16_t frames;
    int16_t startpos;
    animnode_t *anm;
    int16_t curfrm;
    int16_t rendfrm;
    struct hotspots
    {
        int16_t x;
        int16_t y;
        int16_t angles;
        struct directions
        {
            int16_t toframe;
            int16_t angle;
        } directions[CTRL_LEVER_MAX_DIRECTS];
    } hotspots[CTRL_LEVER_MAX_FRAMES];
    int16_t delta_x;
    int16_t delta_y;
    int16_t last_mouse_x;
    int16_t last_mouse_y;
    int16_t mouse_angle;
    int32_t mouse_count;
    bool mouse_captured;
    int32_t hasout[CTRL_LEVER_MAX_FRAMES];                         //seq's frame count
    int32_t outproc[CTRL_LEVER_MAX_FRAMES][CTRL_LEVER_MAX_FRAMES]; //seq's for every frames
    bool autoout;                                                  //seq initiated
    int32_t autoseq;                                               //what frame initiate this seq
    int32_t autoseq_frm;                                           //current seq frame
    int32_t autoseq_time;                                          //time leave to next seq frame
} levernode_t;

typedef struct
{
    int16_t num_states;
    int16_t cur_state;
    animnode_t *anm;
    int32_t center_x;
    int32_t center_y;
    Rect_t rectangle;
    int16_t radius_inner;
    int32_t radius_inner_sq;
    int16_t radius_outer;
    int32_t radius_outer_sq;
    int16_t zero_pointer;
    int16_t start_pointer;
    int32_t cur_frame;
    int32_t to_frame;
    int32_t frame_time;
} safenode_t;

typedef struct
{
    int16_t num_strings;
    char *strings[CTRL_TITLER_MAX_STRINGS];
    Rect_t rectangle;
    SDL_Surface *surface;
    int16_t current_string;
    int16_t next_string;
} titlernode_t;

typedef struct
{
    uint8_t *brush;
    int16_t b_w;
    int16_t b_h;
    SDL_Surface *paint;
    Rect_t rectangle;
    int32_t last_x;
    int32_t last_y;
    int32_t *eligible_objects;
    int32_t eligable_cnt;
    int16_t cursor;
} paintnode_t;

typedef struct
{
    uint32_t fiststatus;
    uint8_t fistnum;
    int16_t cursor;
    int8_t order;
    struct fists_up
    {
        int32_t num_box;
        Rect_t boxes[CTRL_FIST_MAX_BOXES];
    } fists_up[CTRL_FIST_MAX_FISTS];
    struct fists_dwn
    {
        int32_t num_box;
        Rect_t boxes[CTRL_FIST_MAX_BOXES];
    } fists_dwn[CTRL_FIST_MAX_FISTS];

    int32_t num_entries;
    struct entries
    {
        uint32_t strt;
        uint32_t send;
        int32_t anm_str;
        int32_t anm_end;
        int32_t sound;
    } entries[CTRL_FIST_MAX_ENTRS];

    animnode_t *anm;
    Rect_t anm_rect;
    int32_t soundkey;
    int32_t frame_cur;
    int32_t frame_end;
    int32_t frame_time;
    int32_t animation_id;
} fistnode_t;

typedef struct
{
    int32_t num_frames;
    int32_t frame_time;
    int32_t cur_frame;
    int32_t rend_frame;
    int32_t cycle;
    int32_t num_cycles;
    Rect_t *frame_list;
    animnode_t *anm;
    Rect_t rect;
} hotmvnode_t;

typedef struct ctrlnode
{
    int32_t slot;
    int8_t type;
    int32_t venus;
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

int Parse_Control(MList *controlst, mfile_t *fl, char *ctstr);
void ProcessControls(MList *ctrlst);
void Ctrl_DrawControls();
void FlushControlList(MList *lst);
ctrlnode_t *GetControlByID(int32_t id);

#endif // CONTROL_H_INCLUDED
