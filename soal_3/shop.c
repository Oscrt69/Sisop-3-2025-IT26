#include "shop.h"
#include <string.h>
#include <stdio.h>

// Daftar senjata yang tersedia di shop
Weapon weapons[5] = {
    {"Terra Blade", 50, 10, ""},
    {"Flint & Steel", 150, 25, ""},
    {"Kitchen Knife", 200, 35, ""},
    {"Staff of Light", 120, 20, "10% Insta-Kill Chance"},
    {"Dragon Claws", 300, 50, "50% Crit Chance"}
};

Weapon get_weapon(int index) {
    if (index >= 0 && index < 5) {
        return weapons[index];
    }
    return weapons[0]; // Return default weapon if out of bounds
}

const char* get_shop_items() {
    static char shop_list[512];
    shop_list[0] = '\0';
    
    for (int i = 0; i < 5; i++) {
        char item[128];
        snprintf(item, sizeof(item), "[%d] %s - Price: %d gold, Damage: %d", 
                i+1, weapons[i].name, weapons[i].price, weapons[i].damage);
        
        if (strlen(weapons[i].passive)) {
            strcat(item, " (Passive: ");
            strcat(item, weapons[i].passive);
            strcat(item, ")");
        }
        
        strcat(shop_list, item);
        if (i < 4) strcat(shop_list, "|");
    }
    
    return shop_list;
}
