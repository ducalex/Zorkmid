#ifndef PUZZLE_H_INCLUDED
#define PUZZLE_H_INCLUDED

#include "ScriptSystem.h"

struct puzzlenode
{
    uint16_t slot;   //puzzle slot
    MList *CritList; //Criteria list of lists criteria
    MList *ResList;  //results list
    pzllst *owner;
};

typedef struct
{
    int (*func)(char *, int, pzllst *);
    char *param;
    puzzlenode *owner;
    int slot;
} func_node_t;

typedef struct
{
    int32_t slot1;
    int32_t slot2;
    uint8_t oper;
    bool var2; //if true: slot2 is slot; false: slot2 - number
} crit_node_t;

pzllst *CreatePzlLst();
int Parse_Puzzle(pzllst *lst, mfile_t *fl, char *ctstr);
int Puzzle_try_exec(puzzlenode *pzlnod);

void FlushPuzzleList(pzllst *lst);

#endif // PUZZLE_H_INCLUDED
