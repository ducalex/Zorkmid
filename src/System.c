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

dynlist_t *CreateList(size_t blocksize)
{
    dynlist_t *list = NEW(dynlist_t);

    list->blocksize = blocksize;
    list->is_heap = true;

    return list;
}

void ResizeList(dynlist_t *list)
{
    ASSERT(list != NULL);

    if (list->blocksize == 0)
        list->blocksize = 64;

    int new_capacity = list->blocksize * ((list->length / list->blocksize) + 1);
    if (new_capacity != list->capacity)
    {
        LOG_DEBUG("Resizing list %p from %d items to %d items\n", list, list->capacity, new_capacity);
        list->items = realloc(list->items, new_capacity * sizeof(void*));
        list->capacity = new_capacity;
    }
}

void AddToList(dynlist_t *list, void *item)
{
    ASSERT(list != NULL);

    if (list->length + 1 > list->capacity)
        ResizeList(list);

    list->items[list->length] = item;
    list->length++;
}

void DeleteFromList(dynlist_t *list, int index)
{
    ASSERT(list != NULL);

    if (!list->items)
        return;

    if (index >= 0 && index < list->length)
        list->items[index] = NULL;

    // We can only free items at the end of the list
    while (list->length > 0 && list->items[list->length - 1] == NULL)
        list->length--;

    ResizeList(list);
}

void FlushList(dynlist_t *list)
{
    ASSERT(list != NULL);

    list->length = 0;
    ResizeList(list);
}

void DeleteList(dynlist_t *list)
{
    if (list == NULL)
        return;

    DELETE(list->items);

    if (!list->is_heap) // clean up static object
        memset(list, 0, sizeof(dynlist_t));
    else
        DELETE(list);
}
