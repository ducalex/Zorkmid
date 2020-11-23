#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#define MENU_BAR_EXIT 0x1
#define MENU_BAR_SETTINGS 0x2
#define MENU_BAR_RESTORE 0x4
#define MENU_BAR_SAVE 0x8
#define MENU_BAR_ITEM 0x100
#define MENU_BAR_MAGIC 0x200

void Menu_SetVal(uint16_t val);
uint16_t Menu_GetVal();
void Menu_LoadGraphics();
void Menu_Update();
void Menu_Draw();

#endif // MENU_H_INCLUDED
