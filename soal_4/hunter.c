#include "shm_common.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

int current_hunter_idx = -1;
int notification_running = 0;
pthread_t notification_thread;

void print_menu(const char* username) {
    printf("\n=== %s's MENU ===\n", username);
    printf("1. Dungeon List\n");
    printf("2. Dungeon Raid\n");
    printf("3. Hunter's Battle\n");
    printf("4. Notification\n");
    printf("5. Exit\n");
    printf("Choice: ");
    fflush(stdout);  
}

void* notification_loop(void* arg) {
    SystemData* data = (SystemData*)arg;
    while (notification_running) {
        int idx = (data->current_notification_index - 1) % MAX_NOTIF;
        if (idx < 0) idx = MAX_NOTIF - 1;
        
        if (strlen(data->notif[idx]) > 0) {
            printf("\n== NOTIFICATION ==\n%s\n", data->notif[idx]);
            print_menu(data->hunters[current_hunter_idx].username);
        }
        sleep(3);
    }
    return NULL;
}

void toggle_notification(SystemData* data) {
    notification_running = !notification_running;
    if (notification_running) {
        pthread_create(&notification_thread, NULL, notification_loop, data);
        printf("Notifications enabled\n");
    } else {
        pthread_cancel(notification_thread);
        printf("Notifications disabled\n");
    }
}

void list_available_dungeons(SystemData* data, Hunter* h) {
    printf("\n== AVAILABLE DUNGEONS ==\n");
    int count = 0;
    for (int i = 0; i < data->num_dungeons; ++i) {
        if (data->dungeons[i].min_level <= h->level) {
            printf("%d. %s (Level %d+)\n", ++count, data->dungeons[i].name, data->dungeons[i].min_level);
        }
    }
    if (count == 0) {
        printf("No dungeons available for your level!\n");
    }
}

void raid_dungeon(SystemData* data, Hunter* h) {
    list_available_dungeons(data, h);
    
    int choice;
    printf("Choose Dungeon: ");
    scanf("%d", &choice);
    
    int available_count = 0;
    int dungeon_idx = -1;
    for (int i = 0; i < data->num_dungeons; ++i) {
        if (data->dungeons[i].min_level <= h->level) {
            available_count++;
            if (available_count == choice) {
                dungeon_idx = i;
                break;
            }
        }
    }
    
    if (dungeon_idx == -1) {
        printf("Invalid choice!\n");
        return;
    }
    
    Dungeon* d = &data->dungeons[dungeon_idx];
    h->exp += d->exp;
    h->atk += d->atk;
    h->hp += d->hp;
    h->def += d->def;
    
    printf("\nRaid success! Gained:\n");
    printf("ATK: %d\nHP: %d\nDEF: %d\nEXP: %d\n", d->atk, d->hp, d->def, d->exp);
    
    // Check level up
    if (h->exp >= 500) {
        h->level++;
        h->exp = 0;
        printf("\nLEVEL UP! You are now level %d\n", h->level);
    }
    
    // Remove dungeon
    for (int i = dungeon_idx; i < data->num_dungeons - 1; i++) {
        data->dungeons[i] = data->dungeons[i+1];
    }
    data->num_dungeons--;
}

void battle_hunter(SystemData* data, Hunter* attacker) {
    printf("\n--- PVP LIST ---\n");
    for (int i = 0; i < data->num_hunters; ++i) {
        if (i != current_hunter_idx && !data->hunters[i].banned) {
            int power = data->hunters[i].atk + data->hunters[i].hp + data->hunters[i].def;
            printf("%s - Total Power: %d\n", data->hunters[i].username, power);
        }
    }
    
    char target[50];
    printf("Target: ");
    scanf("%49s", target);
    
    int defender_idx = -1;
    for (int i = 0; i < data->num_hunters; ++i) {
        if (strcmp(data->hunters[i].username, target) == 0 && i != current_hunter_idx) {
            defender_idx = i;
            break;
        }
    }
    
    if (defender_idx == -1) {
        printf("Invalid target!\n");
        return;
    }
    
    Hunter* defender = &data->hunters[defender_idx];
    int attacker_power = attacker->atk + attacker->hp + attacker->def;
    int defender_power = defender->atk + defender->hp + defender->def;
    
    printf("Your Power: %d\n", attacker_power);
    printf("Opponent's Power: %d\n", defender_power);
    
    if (attacker_power > defender_power) {
        // Attacker menang
        attacker->atk += defender->atk;
        attacker->hp += defender->hp;
        attacker->def += defender->def;
        
        // Remove defender
        for (int i = defender_idx; i < data->num_hunters - 1; i++) {
            data->hunters[i] = data->hunters[i+1];
        }
        data->num_hunters--;
        
        printf("Battle won! You acquired %s's stats\n", target);
    } else {
        // Defender menang
        defender->atk += attacker->atk;
        defender->hp += attacker->hp;
        defender->def += attacker->def;
        
        // Remove attacker
        for (int i = current_hunter_idx; i < data->num_hunters - 1; i++) {
            data->hunters[i] = data->hunters[i+1];
        }
        data->num_hunters--;
        
        printf("You lost and were eliminated from the system!\n");
        exit(0);
    }
}

void register_hunter(SystemData* data) {
    if (data->num_hunters >= MAX_HUNTERS) {
        printf("Hunter limit reached!\n");
        return;
    }
    
    Hunter* h = &data->hunters[data->num_hunters];
    printf("Username: ");
    scanf("%49s", h->username);
    
    // Default stats
    h->level = 1;
    h->exp = 0;
    h->atk = 10;
    h->hp = 100;
    h->def = 5;
    h->banned = 0;
    h->shm_key = ftok("/tmp", 100 + data->num_hunters);
    
    data->num_hunters++;
    printf("Registration success!\n");
}

int login_hunter(SystemData* data) {
    char username[50];
    printf("Username: ");
    scanf("%49s", username);
    
    for (int i = 0; i < data->num_hunters; ++i) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            if (data->hunters[i].banned) {
                printf("You are banned!\n");
                return -1;
            }
            printf("Welcome %s (Lv:%d)\n", username, data->hunters[i].level);
            return i;
        }
    }
    printf("Hunter not found!\n");
    return -1;
}

int main() {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(SystemData), 0666);
    
    if (shmid == -1) {
        printf("System is not running!\n");
        return 1;
    }
    
    SystemData* data = shmat(shmid, NULL, 0);
    if (data == (void*)-1) {
        perror("shmat failed");
        return 1;
    }

    while (1) {
        int choice;
        if (current_hunter_idx == -1) {
            printf("\nHUNTER MENU\n");
            printf("1. Register\n2. Login\n3. Exit\n");
            printf("Choice: ");
            scanf("%d", &choice);
            
            switch(choice) {
                case 1: register_hunter(data); break;
                case 2: current_hunter_idx = login_hunter(data); break;
                case 3: 
                    shmdt(data);
                    return 0;
                default: printf("Invalid choice!\n");
            }
        } else {
            Hunter* h = &data->hunters[current_hunter_idx];
            printf("\n=== %s's MENU ===\n", h->username);
            printf("1. Dungeon List\n2. Dungeon Raid\n3. Hunter's Battle\n4. Notification\n5. Exit\n");
            printf("Choice: ");
            scanf("%d", &choice);
            
            switch(choice) {
                case 1: 
                list_available_dungeons(data, h); break;
                case 2: 
                raid_dungeon(data, h); break;
                case 3: 
                battle_hunter(data, h); break;
                case 4: 
                toggle_notification(data); break;
                case 5: 
                    current_hunter_idx = -1;
                    if (notification_running) {
                        notification_running = 0;
                        pthread_cancel(notification_thread);
                    }
                    break;
                default: printf("Invalid choice!\n");
            }
        }
    }
    
    shmdt(data);
    return 0;
}
