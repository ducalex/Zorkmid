#ifndef MYLIST_H_INCLUDED
#define MYLIST_H_INCLUDED

#define MLIST_STACK 0x10

//node structure
typedef struct MList_node_s
{
    MList_node_s *next; //pointer to next node
    MList_node_s *prev; //pointer to next node
    void *data;       //pointer to data
    unsigned int idx;
} MList_node;

//List structure
typedef struct
{
    MList_node *CurNode; //pointer to current node
    MList_node *Head;    //pointer to first node
    MList_node *Tail;    //pointer to last node
    unsigned int count;  //count of elements
    unsigned int indx;   //count of elements
    MList_node *Stack[MLIST_STACK];
    unsigned int stkpos;
    bool dontstp;
} MList;

//Linked-list functions
MList *CreateMList();
MList_node *AddToMList(MList *lst, void *item);
void StartMList(MList *lst);
void LastMList(MList *lst);
bool NextSMList(MList *lst);
void NextMList(MList *lst);
void PrevMList(MList *lst);
bool ToIndxMList(MList *lst, unsigned int indx);
void *DataMList(MList *lst);
void DeleteMList(MList *lst);
void FlushMList(MList *lst);
void DeleteCurrent(MList *lst);
bool eofMList(MList *lst);
int getIndxMList(MList *lst);
bool pushMList(MList *lst);
bool popMList(MList *lst);

#endif // MYLIST_H_INCLUDED
