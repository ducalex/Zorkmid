#ifndef ANIMS_H_INCLUDED
#define ANIMS_H_INCLUDED

#include <stdint.h>
#include "Decoder.h"

typedef struct
{
    SDL_Surface **img;
    struct info
    {
        uint32_t w;
        uint32_t h;
        uint32_t time;
        uint32_t frames;
    } info;
} anim_surf_t;

typedef struct
{
    SDL_Surface *img;
    avi_file_t *av;
    bool pld;
    bool loop;
    int32_t lastfrm;
} anim_avi_t;

typedef struct animnode
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    int32_t rel_w;
    int32_t rel_h;
    int32_t start;
    int32_t end;
    int32_t pos;
    int32_t loopcnt;
    int32_t mask;
    int32_t framerate;
    int32_t frames;
    int32_t nexttick;
    int32_t loops;
    anim_surf_t *anim_rlf;
    anim_avi_t *anim_avi;
    int32_t playID;
    bool playing;
} animnode_t;

typedef struct anim_preplay_node
{
    int32_t pointingslot;
    int32_t playerid;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    int32_t start;
    int32_t end;
    int32_t loop;
    animnode_t *point; //pointer for direct access
} anim_preplay_node_t;

action_res_t *Anim_CreateNode(int type);
int Anim_ProcessNode(action_res_t *nod);
int Anim_DeleteNode(action_res_t *nod);

void Anim_Load(animnode_t *nod, char *filename, int u1, int u2, int32_t mask, int framerate);
int Anim_Play(animnode_t *nod, int x, int y, int w, int h, int start, int end, int loop);
void Anim_RenderFrame(animnode_t *mnod, int x, int y, int w, int h, int frame);
void Anim_Process(animnode_t *nod);
void Anim_DeleteAnim(animnode_t *nod);
void Anim_DeleteAnimImage(anim_surf_t *anim);

#endif // ANIMS_H_INCLUDED
