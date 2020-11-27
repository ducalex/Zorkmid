#include "System.h"

static void DeletePuzzleNode(puzzlenode_t *nod)
{
    LOG_DEBUG("Deleting Puzzle #%d\n", nod->slot);

    StartMList(nod->CritList);
    while (!EndOfMList(nod->CritList))
    {
        MList *criteries = (MList *)DataMList(nod->CritList);

        StartMList(criteries);
        while (!EndOfMList(criteries))
        {
            free(DataMList(criteries));
            NextMList(criteries);
        }
        DeleteMList(criteries);

        NextMList(nod->CritList);
    }
    DeleteMList(nod->CritList);

    StartMList(nod->ResList);
    while (!EndOfMList(nod->ResList))
    {
        func_node_t *fun = (func_node_t *)DataMList(nod->ResList);
        if (fun->param != NULL)
            free(fun->param);
        free(fun);
        NextMList(nod->ResList);
    }
    DeleteMList(nod->ResList);

    free(nod);
}

pzllst_t *Puzzle_CreateList(const char *name)
{
    pzllst_t *tmp = NEW(pzllst_t);
    tmp->_list = CreateMList();
    if (name)
        strcpy(tmp->name, name);
    return tmp;
}

void Puzzle_FlushList(pzllst_t *lst)
{
    StartMList(lst->_list);
    while (!EndOfMList(lst->_list))
    {
        DeletePuzzleNode((puzzlenode_t *)DataMList(lst->_list));
        NextMList(lst->_list);
    }

    lst->exec_times = 0;
    lst->stksize = 0;

    FlushMList(lst->_list);
}

static void Parse_Puzzle_Results_Action(char *str, MList *lst)
{
    char buf[255];
    const char *params = " ";
    int slot = 0;
    int len = strlen(str);

    memset(buf, 0, 255);

    for (int i = 0; i < len; i++)
    {
        if (str[i] != '(' && str[i] != 0x20 && str[i] != 0x09 && str[i] != '#' && str[i] != 0x00 && str[i] != ':')
            buf[i] = str[i];
        else
        {
            if (str[i] == ':')
                slot = atoi(str + i + 1);
            params = GetParams(str + i);
            break;
        }
    }

    func_node_t *nod = NEW(func_node_t);
    AddToMList(lst, nod);
    nod->param = strdup(params);
    nod->slot = slot;
    strcpy(nod->action, buf);
}

static int Parse_Puzzle_Flags(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            return 1;
        }
        else if (str_starts_with(str, "once_per_inst"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_ONCE_PER_I);
        }
        else if (str_starts_with(str, "do_me_now"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_DO_ME_NOW);
        }
        else if (str_starts_with(str, "disabled"))
        {
            ScrSys_SetFlag(pzl->slot, ScrSys_GetFlag(pzl->slot) | FLAG_DISABLED);
        }
    }

    return 0;
}

static int Parse_Puzzle_Criteria(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];
    char *str;

    MList *crit_nodes_lst = CreateMList();

    AddToMList(pzl->CritList, crit_nodes_lst);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        str = PrepareString(buf);

        if (str[0] == '}')
        {
            return 1;
        }
        else if (str[0] == '[')
        {
            crit_node_t *nod = NEW(crit_node_t);
            AddToMList(crit_nodes_lst, nod);

            sscanf(&str[1], "%d", &nod->slot1);

            int ij;
            int32_t t_len = strlen(str);
            for (ij = 0; ij < t_len; ij++)
            {
                if (str[ij] == '!')
                {
                    nod->oper = CRIT_OP_NOT;
                    break;
                }
                else if (str[ij] == '>')
                {
                    nod->oper = CRIT_OP_GRE;
                    break;
                }
                else if (str[ij] == '<')
                {
                    nod->oper = CRIT_OP_LEA;
                    break;
                }
                else if (str[ij] == '=')
                {
                    nod->oper = CRIT_OP_EQU;
                    break;
                }
            }

            for (ij++; ij < t_len; ij++)
            {
                if (str[ij] == '[')
                {
                    sscanf(&str[ij + 1], "%d", &nod->slot2);
                    nod->var2 = true;
                    break;
                }
                else if (str[ij] != 0x20 && str[ij] != 0x09)
                {
                    sscanf(&str[ij], "%d", &nod->slot2);
                    nod->var2 = false;
                    break;
                }
            }
        }
        else
        {
            LOG_WARN("Warning!!! %s\n", str);
        }
    }

    return 0;
}

static int Parse_Puzzle_Results(puzzlenode_t *pzl, mfile_t *fl)
{
    char buf[STRBUFSIZE];

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);

        // Format is `action: background: event: other`
        char *str = PrepareString(buf);
        char *str2;

        if (str[0] == '}')
            return 1;
        else if ((str2 = strchr(str, ':')))
            Parse_Puzzle_Results_Action(str2 + 1, pzl->ResList);
        else
            LOG_WARN("Unknown result action: %s\n", str);
    }

    return 0;
}

void Puzzle_Parse(pzllst_t *lst, mfile_t *fl, char *ctstr)
{
    char buf[STRBUFSIZE];
    uint32_t slot;

    sscanf(ctstr, "puzzle:%d", &slot);

    puzzlenode_t *pzl = NEW(puzzlenode_t);
    pzl->owner = lst;
    pzl->slot = slot;
    pzl->CritList = CreateMList();
    pzl->ResList = CreateMList();

    ScrSys_SetFlag(pzl->slot, 0);

    while (!mfeof(fl))
    {
        mfgets(buf, STRBUFSIZE, fl);
        char *str = PrepareString(buf);

        if (str[0] == '}') // We've reached the end!
        {
            if ((ScrSys_GetFlag(pzl->slot) & FLAG_ONCE_PER_I))
                SetgVarInt(slot, 0);

            LOG_DEBUG("Created Puzzle %d\n", slot);
            AddToMList(lst->_list, pzl);
            return;
        }
        else if (str_starts_with(str, "criteria"))
        {
            Parse_Puzzle_Criteria(pzl, fl);
        }
        else if (str_starts_with(str, "results"))
        {
            Parse_Puzzle_Results(pzl, fl);
        }
        else if (str_starts_with(str, "flags"))
        {
            Parse_Puzzle_Flags(pzl, fl);
        }
    }

    // If we reach that point the parsing failed...
    LOG_WARN("Failed to parse puzzle '%s'...", ctstr);
    DeletePuzzleNode(pzl);
}

static bool ProcessCriteries(MList *lst)
{
    bool tmp = true;

    StartMList(lst);
    while (!EndOfMList(lst))
    {
        crit_node_t *critnd = (crit_node_t *)DataMList(lst);

        LOG_DEBUG("  [%d] %d [%d] %d\n", critnd->slot1, critnd->oper, critnd->slot2, critnd->var2);

        int tmp1 = GetgVarInt(critnd->slot1);
        int tmp2 = critnd->var2 ? GetgVarInt(critnd->slot2) : critnd->slot2;

        switch (critnd->oper)
        {
        case CRIT_OP_EQU:
            tmp &= (tmp1 == tmp2);
            break;
        case CRIT_OP_GRE:
            tmp &= (tmp1 > tmp2);
            break;
        case CRIT_OP_LEA:
            tmp &= (tmp1 < tmp2);
            break;
        case CRIT_OP_NOT:
            tmp &= (tmp1 != tmp2);
            break;
        }

        if (!tmp)
            break;

        NextMList(lst);
    }
    return tmp;
}

int Puzzle_TryExec(puzzlenode_t *pzlnod) //, pzllst_t *owner)
{
    if (ScrSys_GetFlag(pzlnod->slot) & FLAG_DISABLED)
        return ACTION_NORMAL;

    if (GetgVarInt(pzlnod->slot) == 1)
        return ACTION_NORMAL;

    if (pzlnod->owner->exec_times == 0)
        if (!(ScrSys_GetFlag(pzlnod->slot) & FLAG_DO_ME_NOW))
            return ACTION_NORMAL;

    if (pzlnod->CritList->count > 0)
    {
        bool match = false;

        StartMList(pzlnod->CritList);
        while (!EndOfMList(pzlnod->CritList))
        {
            MList *criteries = (MList *)DataMList(pzlnod->CritList);

            if (ProcessCriteries(criteries))
            {
                match = true;
                break;
            }

            NextMList(pzlnod->CritList);
        }
        if (!match)
            return ACTION_NORMAL;
    }

    LOG_DEBUG("Running puzzle %d (%s)\n", pzlnod->slot, pzlnod->owner->name);

    SetgVarInt(pzlnod->slot, 1);

    StartMList(pzlnod->ResList);
    while (!EndOfMList(pzlnod->ResList))
    {
        func_node_t *fun = (func_node_t *)DataMList(pzlnod->ResList);
        if (Actions_Run(fun->action, fun->param, fun->slot, pzlnod->owner) == ACTION_BREAK)
        {
            return ACTION_BREAK;
        }
        NextMList(pzlnod->ResList);
    }

    return ACTION_NORMAL;
}
