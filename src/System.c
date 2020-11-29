#include "System.h"

char *PrepareString(char *buf)
{
    char *str = (char *)str_ltrim(buf);
    char *tmp;

    // Cut at newline or comment
    if ((tmp = strchr(str, 0x0A))) *tmp = 0;
    if ((tmp = strchr(str, 0x0D))) *tmp = 0;
    if ((tmp = strchr(str, '#'))) *tmp = 0;

    for (int i = 0, len = strlen(str); i < len; i++)
        str[i] = tolower(str[i]);

    return str;
}

char *GetParams(char *str)
{
    for (int i = strlen(str) - 1; i > -1; i--)
    {
        if (str[i] == ')')
            str[i] = 0x0;
        else if (str[i] == '(')
        {
            return str + i + 1;
        }
        else if (str[i] == ',')
            str[i] = ' ';
    }
    return (char *)" ";
}

const char *str_find(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return NULL;

    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);

    if (n_len > h_len)
        return NULL;

    const char *a = haystack, *e = needle;

    while (*a && *e)
    {
        if (toupper((unsigned char)(*a)) != toupper((unsigned char)(*e)))
        {
            ++haystack;
            a = haystack;
            e = needle;
        }
        else
        {
            ++a;
            ++e;
        }
    }

    return *e ? NULL : haystack;
}

bool str_starts_with(const char *haystack, const char *needle)
{
    return str_find(haystack, needle) == haystack;
}

bool str_ends_with(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return false;

    size_t h_len = strlen(haystack);
    size_t n_len = strlen(needle);

    if (n_len > h_len)
        return false;

    return str_find(haystack + h_len - n_len, needle) != NULL;
}

bool str_equals(const char *str1, const char *str2)
{
    return str1 && str2 && strcasecmp(str1, str2) == 0;
}

bool str_empty(const char *str)
{
    return str == NULL || str[0] == 0;
}

const char *str_ltrim(const char *str)
{
    if (!str_empty(str))
    {
        while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
            str++;
    }
    return str;
}

char *str_trim(const char *buffer)
{
    if (!str_empty(buffer))
    {
        const char *str = str_ltrim(buffer);
        const char *tmp = str + strlen(str) - 1;
        size_t trim = 0;

        while (*tmp == ' ' || *tmp == '\t' || *tmp == '\r' || *tmp == '\n')
        {
            tmp--;
            trim++;
        }

        int new_len = strlen(str) - trim;

        if (new_len > 0) {
            char *out = NEW_ARRAY(char, new_len + 1);
            memcpy(out, str, new_len);
            out[new_len] = 0;
            return out;
        }
    }

    return strdup("");
}

//Creates single linked-list object
MList *CreateMList()
{
    return NEW(MList);
}

//Adds item to linked-list
MList_node *AddToMList(MList *lst, void *item)
{
    MList_node *tmp = NEW(MList_node);
    tmp->data = item;
    tmp->next = NULL;
    tmp->prev = lst->Tail;
    tmp->idx = lst->indx;
    if (lst->count == 0)
    {
        lst->Head = tmp;
        lst->CurNode = tmp;
        lst->Tail = tmp;
    }
    else
    {
        lst->Tail->next = tmp;
        lst->Tail = tmp;
    }

    lst->count++;
    lst->indx++;

    return tmp;
}

//Go to the first linked-list item
void StartMList(MList *lst)
{
    lst->CurNode = lst->Head;
    lst->dontstp = false;
}

void LastMList(MList *lst)
{
    lst->CurNode = lst->Tail;
    lst->dontstp = false;
}

//Go to next linked-list item without checking of item exist's
void NextMList(MList *lst)
{
    if (lst->CurNode)
        if (!lst->dontstp)
            lst->CurNode = lst->CurNode->next;

    lst->dontstp = false;
}

//Go to prev linked-list item without checking of item exist's
void PrevMList(MList *lst)
{
    lst->dontstp = false;
    if (lst->CurNode)
        lst->CurNode = lst->CurNode->prev;
}

//Get data of element
void *DataMList(MList *lst)
{
    return lst->CurNode->data;
}

//Delete list object and delete all nodes assigned to list
void DeleteMList(MList *lst)
{
    if (lst->count > 0)
    {
        MList_node *nxt = lst->Head->next;
        lst->CurNode = lst->Head;
        while (lst->CurNode)
        {
            nxt = lst->CurNode->next;
            free(lst->CurNode);
            lst->CurNode = nxt;
        }
    }
    free(lst);
}

void FlushMList(MList *lst)
{
    if (lst->count > 0)
    {
        MList_node *nxt = lst->Head->next;
        lst->CurNode = lst->Head;
        while (lst->CurNode)
        {
            nxt = lst->CurNode->next;
            free(lst->CurNode);
            lst->CurNode = nxt;
        }
    }

    lst->CurNode = NULL;
    lst->Head = NULL;
    lst->Tail = NULL;
    lst->count = 0;
    lst->indx = 0;
    lst->stkpos = 0;
    lst->dontstp = false;
}

void DeleteCurrent(MList *lst)
{
    if (lst->stkpos != 0)
        Z_PANIC("???");

    lst->dontstp = false;

    if (lst->CurNode->next)
        lst->CurNode->next->prev = lst->CurNode->prev;

    if (lst->CurNode->prev)
        lst->CurNode->prev->next = lst->CurNode->next;

    if (lst->CurNode == lst->Tail)
        lst->Tail = lst->CurNode->prev;

    if (lst->CurNode == lst->Head)
    {
        lst->Head = lst->CurNode->next;
        lst->dontstp = true;
    }

    MList_node *nod;

    if (lst->CurNode->prev)
        nod = lst->CurNode->prev;
    else
        nod = lst->Head;

    free(lst->CurNode);

    lst->stkpos = 0; //Clean Stack!
    lst->CurNode = nod;
    lst->count--;

    if (lst->count == 0)
    {
        lst->CurNode = NULL;
        lst->Head = NULL;
        lst->Tail = NULL;
    }
}

bool PushMList(MList *lst)
{
    if (lst->stkpos >= MLIST_STACK)
        return false;

    lst->Stack[lst->stkpos] = lst->CurNode;
    lst->stkpos++;

    return true;
}

bool PopMList(MList *lst)
{
    if (lst->stkpos <= 0)
    {
        lst->CurNode = lst->Head;
        return false;
    }

    lst->stkpos--;
    lst->CurNode = lst->Stack[lst->stkpos];

    return true;
}

//Return true on EOF of list
bool EndOfMList(MList *lst)
{
    return (lst->CurNode == NULL);
}
