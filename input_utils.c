
#include "input_utils.h"

#if 0
void GB_Input_Update(void)
{
    Keys[0] = 0; Keys[1] = 0; Keys[2] = 0; Keys[3] = 0;

    if(Keys_Down[VK_LEFT]) Keys[0] |= KEY_LEFT;
    if(Keys_Down[VK_UP]) Keys[0] |= KEY_UP;
    if(Keys_Down[VK_RIGHT]) Keys[0] |= KEY_RIGHT;
    if(Keys_Down[VK_DOWN]) Keys[0] |= KEY_DOWN;
    if(Keys_Down['X']) Keys[0] |= KEY_A;
    if(Keys_Down['Z']) Keys[0] |= KEY_B;
    if(Keys_Down[VK_RETURN]) Keys[0] |= KEY_START;
    if(Keys_Down[VK_SHIFT]) Keys[0] |= KEY_SELECT;

    if(SGB_MultiplayerIsEnabled())
    {
        int i;
        for(i = 1; i < 4; i++)
        {
            if(keystate[Config_Controls_Get_Key(i,P_KEY_LEFT)]) Keys[i] |= KEY_LEFT;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_UP)]) Keys[i] |= KEY_UP;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_RIGHT)]) Keys[i] |= KEY_RIGHT;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_DOWN)]) Keys[i] |= KEY_DOWN;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_A)]) Keys[i] |= KEY_A;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_B)]) Keys[i] |= KEY_B;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_START)]) Keys[i] |= KEY_START;
            if(keystate[Config_Controls_Get_Key(i,P_KEY_SELECT)]) Keys[i] |= KEY_SELECT;
        }
    }

    if(GameBoy.Emulator.MemoryController == MEM_MBC7)
    {
        GB_Input_Update_MBC7(Keys_Down[VK_NUMPAD8],Keys_Down[VK_NUMPAD2],
                            Keys_Down[VK_NUMPAD6],Keys_Down[VK_NUMPAD4]);
    }

}
#endif


//void GB_InputSet(int player, int a, int b, int st, int se, int r, int l, int u, int d);
//void GB_InputSetMBC7Joystick(int x, int y); // -200 to 200
//void GB_InputSetMBC7Buttons(int up, int down, int right, int left);
