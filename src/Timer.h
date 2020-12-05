#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

typedef struct struct_action_res action_res_t;

action_res_t *Timer_CreateNode();
int Timer_DeleteNode(action_res_t *nod);
int Timer_ProcessNode(action_res_t *nod);

#endif
