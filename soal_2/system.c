#include "shm_common.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

void add_notification(SystemData* data, const char* message) {
    strncpy(data->notif[data->current_notification_index], message, 99);
    data->notif[data->current_notification_index][99] = '\0';
    data->current_notification_index = (data->current_notification_index + 1) % MAX_NOTIF;
}

void generate_dungeon(SystemData* data) {
    if (data->num_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon limit reached!\n");
        return;
    }

    Dungeon* d = &data->dungeons[data->num_dungeons];

    const char* dungeon_names[] = {
        "Double Dungeon",
        "Demon Castle",
        "Pyramid Dungeon",
        "Red Gate Dungeon",
        "Hunters Guild Dungeon",
        "Busan A-Rank Dungeon",
        "Insects Dungeon",
        "Goblins Dungeon",
        "D-Rank Dungeon",
        "Gwanak Mountain Dungeon",
        "Hapjeong Subway Station Dungeon"
    };
    
   
    strncpy(d->name, dungeon_names[rand() % (sizeof(dungeon_names)/sizeof(dungeon_names[0]))], 49);
    d->name[49] = '\0';
    
    d->min_level = rand() % 5 + 1;   // Level minimal 1-5
    d->exp = rand() % 151 + 150;    // ATK 150-300
    d->atk = rand() % 51 + 100;     // HP 100-150
    d->hp = rand() % 51 + 50;       // DEF 50-100
    d->def = rand() % 26 + 25;      // EXP 25-50
    d->shm_key = ftok("/tmp", 200 + data->num_dungeons);
    
    data->num_dungeons++;
    
    printf("\nDungeon Generated!\n");
    printf("Name: %s\n", d->name);
    printf("Min Level: %d\n", d->min_level);
    char notif_msg[100];
    snprintf(notif_msg, sizeof(notif_msg), 
            "New dungeon: %s (Lv. %d)", d->name, d->min_level);
    add_notification(data, notif_msg);
}

void list_hunters(SystemData* data) {
    printf("\n== HUNTER INFO ==\n");
    for (int i = 0; i < data->num_hunters; ++i) {
        Hunter* h = &data->hunters[i];
        printf("Name: %-10s Level: %-2d EXP: %-3d ATK: %-3d HP: %-3d DEF: %-3d %s\n",
               h->username, h->level, h->exp, h->atk, h->hp, h->def,
               h->banned ? "[BANNED]" : "");
    }
}

void list_dungeons(SystemData* data) {
    printf("\n== DUNGEON INFO ==\n");
    for (int i = 0; i < data->num_dungeons; ++i) {
        Dungeon* d = &data->dungeons[i];
        printf("[Dungeon %d]\n", i+1);
        printf("Name: %s\n", d->name);
        printf("Minimum Level: %d\n", d->min_level);
        printf("EXP Reward: %d\n", d->exp);
        printf("ATK: %d\n", d->atk);
        printf("HP: %d\n", d->hp);
        printf("DEF: %d\n", d->def);
        printf("Key: %d\n\n", d->shm_key);
    }
}

void ban_hunter(SystemData* data) {
    char username[50];
    printf("Enter hunter username to ban/unban: ");
    scanf("%49s", username);
    
    for (int i = 0; i < data->num_hunters; ++i) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            data->hunters[i].banned = !data->hunters[i].banned;
            printf("%s has been %s\n", username, 
                  data->hunters[i].banned ? "BANNED" : "UNBANNED");
            return;
        }
    }
    printf("Hunter not found!\n");
}

void reset_hunter(SystemData* data) {
    char username[50];
    printf("Enter hunter username to reset: ");
    scanf("%49s", username);
    
    for (int i = 0; i < data->num_hunters; ++i) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            data->hunters[i].level = 1;
            data->hunters[i].exp = 0;
            data->hunters[i].atk = 10;
            data->hunters[i].hp = 100;
            data->hunters[i].def = 5;
            printf("%s stats have been reset\n", username);
            return;
        }
    }
    printf("Hunter not found!\n");
}

int main() {
    srand(time(NULL));
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(SystemData), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    SystemData* data = shmat(shmid, NULL, 0);
    if (data == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    if (data->num_hunters == 0 && data->num_dungeons == 0) {
        memset(data, 0, sizeof(SystemData));
        data->current_notification_index = 0;
    }

    while (1) {
        int choice;
        printf("\n=== SYSTEM MENU ===");
        printf("\n1. Hunter Info\n2. Dungeon Info\n3. Generate Dungeon\n4. Ban Hunter\n5. Reset Hunter\n6. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: list_hunters(data); break;
            case 2: list_dungeons(data); break;
            case 3: generate_dungeon(data); break;
            case 4: ban_hunter(data); break;
            case 5: reset_hunter(data); break;
            case 6: 
                shmdt(data);
                shmctl(shmid, IPC_RMID, NULL);
                exit(0);
            default: printf("Invalid choice!\n");
        }
    } 
    
    return 0;
}
