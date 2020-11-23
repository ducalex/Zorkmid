#ifndef ACTIONS_H_INCLUDED
#define ACTIONS_H_INCLUDED

#include "System.h"

#define ACTION_NOT_FOUND 1
#define ACTION_NORMAL 0
#define ACTION_ERROR -1
#define ACTION_BREAK -2

int action_exec(const char* name, char *params, int aSlot, pzllst_t *owner);

#endif // ACTIONS_H_INCLUDED
