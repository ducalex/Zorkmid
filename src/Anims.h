#ifndef ANIMS_H_INCLUDED
#define ANIMS_H_INCLUDED

#include "System.h"

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
    int32_t loopcnt;
    int32_t mask;
    int32_t framerate;
    int32_t CurFr;
    int32_t nexttick;
    int32_t loops;
    int32_t type; // AVI or RLF or MPG
    anim_surf_t *anim_rlf;
    anim_avi_t *anim_avi;
    scaler_t *scal;
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

action_res_t *Anim_CreateAnimPlayNode();
action_res_t *Anim_CreateAnimPreNode();
action_res_t *Anim_CreateAnimPlayPreNode();

void Anim_Process(animnode_t *nod);
void Anim_Load(animnode_t *nod, char *filename, int u1, int u2, int32_t mask, int framerate);
void anim_DeleteAnim(animnode_t *nod);
int Anim_Play(animnode_t *nod, int x, int y, int w, int h, int start, int end, int loop);
int Anim_ProcessPlayNode(action_res_t *nod);
int Anim_ProcessPreNode(action_res_t *nod);
int Anim_ProcessPrePlayNode(action_res_t *nod);
int anim_DeleteAnimPlay(action_res_t *nod);
int anim_DeleteAnimPreNod(action_res_t *nod);
int anim_DeleteAnimPrePlayNode(action_res_t *nod);
void Anim_RenderFrame(animnode_t *mnod, int16_t x, int16_t y, int16_t w, int16_t h, int16_t frame);

#endif // ANIMS_H_INCLUDED
