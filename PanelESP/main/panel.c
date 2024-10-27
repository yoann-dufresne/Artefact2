

#include "panel.h"


int hardware_to_logical_button(int hardware_idx)
{
    if (PANEL_ID % 2 == 0)
        return (hardware_idx + PANEL_ID) % 8;
    else
        return (hardware_idx + PANEL_ID + 5) % 8;
}


int logical_to_hardware_button(int logical_idx)
{
    if (PANEL_ID % 2 == 0)
        return (logical_idx - PANEL_ID + 8) % 8;
    else
        return (logical_idx - PANEL_ID + 11) % 8;
}
