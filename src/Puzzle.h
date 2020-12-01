#ifndef PUZZLE_H_INCLUDED
#define PUZZLE_H_INCLUDED

typedef struct
{
    uint16_t slot;   //puzzle slot
    MList CritList; //Criteria list of lists criteria
    MList ResList;  //results list
    pzllst_t *owner;
} puzzlenode_t;

typedef struct
{
    int32_t slot1;
    int32_t slot2;
    uint8_t oper;
    bool var2; //if true: slot2 is slot; false: slot2 - number
} crit_node_t;

pzllst_t *Puzzle_CreateList(const char *name);
void Puzzle_FlushList(pzllst_t *lst);
void Puzzle_Parse(pzllst_t *lst, mfile_t *fl, char *ctstr);
int Puzzle_TryExec(puzzlenode_t *pzlnod);

#endif // PUZZLE_H_INCLUDED
