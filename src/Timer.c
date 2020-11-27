#include "System.h"

action_res_t *Timer_CreateNode()
{
    action_res_t *tmp = NEW(action_res_t);

    tmp->node_type = NODE_TYPE_TIMER;
    tmp->need_delete = false;

    return tmp;
}

int Timer_DeleteNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TIMER)
        return NODE_RET_NO;

    if (nod->nodes.node_timer < 0)
        SetgVarInt(nod->slot, 2);
    else
        SetgVarInt(nod->slot, nod->nodes.node_timer);

    SetGNode(nod->slot, NULL);

    free(nod);

    return NODE_RET_DELETE;
}

int Timer_ProcessNode(action_res_t *nod)
{
    if (nod->node_type != NODE_TYPE_TIMER)
        return NODE_RET_OK;

    if (nod->nodes.node_timer < 0)
    {
        Timer_DeleteNode(nod);
        return NODE_RET_DELETE;
    }

    nod->nodes.node_timer -= GetDTime();

    return NODE_RET_OK;
}
