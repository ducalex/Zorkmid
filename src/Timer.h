#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include "System.h"

action_res_t *Timer_CreateNode();
int Timer_DeleteNode(action_res_t *nod);
int Timer_ProcessNode(action_res_t *nod);

#endif
