#ifndef ACTIONS_H_INCLUDED
#define ACTIONS_H_INCLUDED

#define ACTION_NOT_FOUND 1
#define ACTION_NORMAL 0
#define ACTION_ERROR -1
#define ACTION_BREAK -2

int Actions_Run(const char* name, char *params, int aSlot, int owner);

#endif // ACTIONS_H_INCLUDED
