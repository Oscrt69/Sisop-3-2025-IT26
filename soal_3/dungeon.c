#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>
#include "shop.h"

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int gold;
    char equipped_weapon[50];
    int base_damage;
    int kills;
    int weapons_owned[5];
    int socket;
    int in_battle;
} Player;

Player players[MAX_CLIENTS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void send_response(int sock, const char* response) {
    send(sock, response, strlen(response), 0);
}

void* handle_client(void* arg) {
    int client_socket = *((int*)arg);
    char buffer[BUFFER_SIZE] = {0};
    int player_index = -1;

    // Initialize player
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (players[i].socket == 0) {
            player_index = i;
            players[i].socket = client_socket;
            players[i].gold = 500;
            strcpy(players[i].equipped_weapon, "Fists");
            players[i].base_damage = 5;
            players[i].kills = 0;
            players[i].in_battle = 0;
            memset(players[i].weapons_owned, 0, sizeof(players[i].weapons_owned));
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    if (player_index == -1) {
        send_response(client_socket, "SERVER_FULL");
        close(client_socket);
        return NULL;
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int valread = read(client_socket, buffer, BUFFER_SIZE);
        if (valread <= 0) break;

        char response[BUFFER_SIZE] = {0};
        char command[20], param[50];
        sscanf(buffer, "%s %s", command, param);

        pthread_mutex_lock(&mutex);
        Player* p = &players[player_index];

        if (strcmp(command, "STATS") == 0) {
            snprintf(response, BUFFER_SIZE, "STATS|GOLD:%d|WEAPON:%s|DAMAGE:%d|KILLS:%d|IN_BATTLE:%d", 
                    p->gold, p->equipped_weapon, p->base_damage, p->kills, p->in_battle);
        }
        else if (strcmp(command, "SHOP") == 0) {
            snprintf(response, BUFFER_SIZE, "SHOP|%s", get_shop_items());
        }
        else if (strcmp(command, "BUY") == 0) {
            if (p->in_battle) {
                strcpy(response, "ERROR|Cannot shop while in battle");
            } else {
                int weapon_id = atoi(param);
                if (weapon_id < 1 || weapon_id > 5) {
                    strcpy(response, "ERROR|Invalid weapon ID");
                } else {
                    Weapon w = get_weapon(weapon_id - 1);
                    if (p->gold < w.price) {
                        strcpy(response, "ERROR|Not enough gold");
                    } else if (p->weapons_owned[weapon_id - 1]) {
                        strcpy(response, "ERROR|You already own this weapon");
                    } else {
                        p->gold -= w.price;
                        p->weapons_owned[weapon_id - 1] = 1;
                        snprintf(response, BUFFER_SIZE, "SUCCESS|Purchased %s! Remaining gold: %d", 
                                w.name, p->gold);
                    }
                }
            }
        }
        else if (strcmp(command, "INVENTORY") == 0) {
            char inventory[512] = {0};
            strcat(inventory, "INVENTORY|");
            
            // Always include fists
            strcat(inventory, "0:Fists|");
            
            for (int i = 0; i < 5; i++) {
                if (p->weapons_owned[i]) {
                    Weapon w = get_weapon(i);
                    char item[128];
                    snprintf(item, sizeof(item), "%d:%s", i+1, w.name);
                    if (w.passive[0] != '\0') {
                        strcat(item, ":");
                        strcat(item, w.passive);
                    }
                    if (strcmp(p->equipped_weapon, w.name) == 0) {
                        strcat(item, ":EQUIPPED");
                    }
                    strcat(inventory, item);
                    strcat(inventory, "|");
                }
            }
            strcpy(response, inventory);
        }
        else if (strcmp(command, "EQUIP") == 0) {
            if (p->in_battle) {
                strcpy(response, "ERROR|Cannot change weapons during battle");
            } else {
                int weapon_id = atoi(param);
                if (weapon_id == 0) {
                    strcpy(p->equipped_weapon, "Fists");
                    p->base_damage = 5;
                    strcpy(response, "SUCCESS|Equipped Fists");
                } 
                else if (weapon_id > 0 && weapon_id <= 5 && p->weapons_owned[weapon_id - 1]) {
                    Weapon w = get_weapon(weapon_id - 1);
                    strcpy(p->equipped_weapon, w.name);
                    p->base_damage = w.damage;
                    snprintf(response, BUFFER_SIZE, "SUCCESS|Equipped %s", w.name);
                } 
                else {
                    strcpy(response, "ERROR|Invalid weapon or you don't own it");
                }
            }
        }
        else if (strcmp(command, "BATTLE") == 0) {
            if (p->in_battle) {
                strcpy(response, "ERROR|Already in battle");
            } else {
                p->in_battle = 1;
                int enemy_hp = 50 + rand() % 151; // 50-200 HP
                int reward = 50 + rand() % 151;   // 50-200 gold
                
                // Store battle data in response
                snprintf(response, BUFFER_SIZE, "BATTLE_START|ENEMY_HP:%d|REWARD:%d", 
                        enemy_hp, reward);
            }
        }
        else if (strcmp(command, "ATTACK") == 0) {
            if (!p->in_battle) {
                strcpy(response, "ERROR|Not in battle");
            } else {
                // Parse current enemy HP and reward from param
                int enemy_hp, reward;
                sscanf(param, "ENEMY_HP:%d|REWARD:%d", &enemy_hp, &reward);
                
                int damage = p->base_damage + (rand() % (p->base_damage / 2 + 1));
                int is_critical = 0;
                
                // Critical calculation
                if (strstr(p->equipped_weapon, "Dragon Claws") != NULL && rand() % 100 < 50) {
                    damage *= 2;
                    is_critical = 1;
                } else if (strstr(p->equipped_weapon, "Staff of Light") != NULL && rand() % 100 < 10) {
                    damage = enemy_hp;
                    is_critical = 2;
                } else if (rand() % 100 < 15) {
                    damage *= 2;
                    is_critical = 1;
                }
                
                enemy_hp -= damage;
                if (enemy_hp < 0) enemy_hp = 0;
                
                if (enemy_hp == 0) {
                    p->kills++;
                    p->gold += reward;
                    p->in_battle = 0;
                    snprintf(response, BUFFER_SIZE, 
                            "BATTLE_RESULT|VICTORY|DAMAGE:%d|CRITICAL:%d|REWARD:%d", 
                            damage, is_critical, reward);
                } else {
                    snprintf(response, BUFFER_SIZE, 
                            "BATTLE_UPDATE|DAMAGE:%d|CRITICAL:%d|ENEMY_HP:%d|REWARD:%d", 
                            damage, is_critical, enemy_hp, reward);
                }
            }
        }
        else if (strcmp(command, "FLEE") == 0) {
            if (p->in_battle) {
                p->in_battle = 0;
                strcpy(response, "BATTLE_RESULT|FLED");
            } else {
                strcpy(response, "ERROR|Not in battle");
            }
        }
        else if (strcmp(command, "EXIT") == 0) {
            break;
        }
        else {
            strcpy(response, "ERROR|Invalid command");
        }
        
        pthread_mutex_unlock(&mutex);
        send_response(client_socket, response);
    }

    // Cleanup
    close(client_socket);
    pthread_mutex_lock(&mutex);
    players[player_index].socket = 0;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    srand(time(NULL));

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Dungeon server running on port %d...\n", PORT);
    
    // Accept connections
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)&new_socket) < 0) {
            perror("could not create thread");
            close(new_socket);
            continue;
        }
        pthread_detach(thread_id);
    }
    
    return 0;
}
