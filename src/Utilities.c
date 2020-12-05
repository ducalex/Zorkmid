#include "Utilities.h"

static const uint8_t PT[] = {
    44, 114, 112, 167, 188, 142, 163, 43, 216, 251, 243, 198, 228, 119, 70, 196, 38, 102, 219,
    120, 61, 88, 131, 8, 147, 93, 69, 174, 0, 249, 211, 234, 33, 239, 74, 195, 14, 80, 51, 92,
    165, 26, 193, 50, 100, 42, 11, 149, 108, 59, 64, 7, 181, 104, 191, 83, 126, 205, 226, 210,
    1, 90, 123, 231, 161, 109, 101, 6, 107, 85, 162, 232, 246, 150, 76, 84, 63, 138, 137, 247,
    10, 2, 183, 236, 65, 140, 94, 46, 19, 9, 32, 194, 86, 153, 41, 201, 133, 4, 245, 115, 158,
    29, 25, 206, 212, 75, 227, 67, 49, 229, 189, 116, 21, 235, 238, 16, 129, 23, 222, 220, 87,
    134, 39, 121, 230, 135, 73, 182, 13, 35, 254, 164, 248, 145, 213, 122, 185, 233, 113, 152,
    105, 192, 169, 97, 127, 156, 139, 66, 98, 209, 240, 171, 151, 154, 103, 3, 81, 53, 55, 82,
    159, 48, 72, 203, 96, 187, 27, 190, 89, 18, 255, 237, 242, 79, 250, 60, 91, 136, 166, 170,
    15, 31, 58, 244, 186, 176, 78, 157, 224, 40, 199, 225, 68, 45, 110, 252, 178, 20, 30, 200,
    28, 99, 56, 5, 62, 37, 17, 184, 12, 221, 146, 57, 36, 168, 34, 180, 24, 148, 173, 118, 54,
    197, 125, 223, 253, 208, 132, 202, 141, 117, 214, 144, 71, 124, 155, 47, 128, 95, 111, 52,
    106, 22, 218, 160, 172, 77, 175, 217, 143, 179, 177, 207, 215, 130, 241, 204
};

uint8_t hash_l(const char *str)
{
    uint8_t *bytes = (uint8_t *)str;
    uint8_t hash = 0, c;

    while ((c = *bytes++))
        hash = PT[(uint8_t)(hash ^ tolower(c))];

    return hash;
}

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
