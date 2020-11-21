#include "System.h"

static int get_count()
{
    return GetgVarInt(SLOT_INV_STORAGE_S1);
}

static void set_count(int n)
{
    SetgVarInt(SLOT_INV_STORAGE_S1, n);
}

static int get_item(int index)
{
    if (index < 49 && index >= 0)
        return GetgVarInt(SLOT_INV_STORAGE_0 + index);
    return -1;
}

static void set_item(int index, int item)
{
    if (index < 49 && index >= 0)
        SetgVarInt(SLOT_INV_STORAGE_0 + index, item);
}

static void equip_item(int n)
{
    SetgVarInt(SLOT_INVENTORY_MOUSE, get_item(n));
}

void inv_drop(int item)
{
    int items_cnt = get_count();

    if (!items_cnt)
        return;

    int index = 0;

    //finding needed item
    while (index < items_cnt)
    {
        if (get_item(index) == item)
            break;

        index++;
    }

    // if item in the inventory
    if (items_cnt != index)
    {
        //shift all items left with rewrite founded item
        for (int v = index; v < items_cnt - 1; v++)
            set_item(v, get_item(v + 1));

        //del last item
        set_item(items_cnt - 1, 0);
        set_count(get_count() - 1);
        equip_item(0);
    }
}

void inv_add(int item)
{
    int cnt = get_count();

    if (cnt < 49)
    {
        bool flag = 1;

        if (cnt == 0)
        {
            set_item(0, 0);
            set_count(1);
            cnt = 1;
        }

        for (int cur = 0; cur < cnt; cur++)
            if (get_item(cur) == item)
            {
                flag = 0;
                break;
            }

        if (flag)
        {
            for (int i = cnt; i > 0; --i)
                set_item(i, get_item(i - 1));
            set_item(0, item);
            set_count(cnt + 1);
            equip_item(0);
        }
    }
}

void inv_cycle()
{
    int item_cnt = get_count();
    int cur_item = get_item(0);
    if (item_cnt > 1)
    {
        for (int i = 0; i < item_cnt - 1; i++)
            set_item(i, get_item(i + 1));
        set_item(item_cnt - 1, cur_item);
        equip_item(0);
    }
}
