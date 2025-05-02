#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int sock;

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void display_menu() {
    printf("\n==== MAIN MENU ====\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapons)\n");
    printf("3. View Inventory & Equip Weapons\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("\nChoose an option: ");
}

void show_stats() {
    char buffer[BUFFER_SIZE] = {0};
    send(sock, "STATS", strlen("STATS"), 0);
    int bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        printf("\nFailed to get stats from server\n");
        return;
    }
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "STATS|", 6) != 0) {
        printf("\nInvalid stats format received: %s\n", buffer);
        return;
    }
    
    // Parse response
    int gold, damage, kills, in_battle;
    char weapon[50] = {0};
    char* token = strtok(buffer + 6, "|");
    while (token != NULL) {
        if (sscanf(token, "GOLD:%d", &gold) == 1) {}
        else if (sscanf(token, "WEAPON:%49[^|]", weapon) == 1) {}
        else if (sscanf(token, "DAMAGE:%d", &damage) == 1) {}
        else if (sscanf(token, "KILLS:%d", &kills) == 1) {}
        else if (sscanf(token, "IN_BATTLE:%d", &in_battle) == 1) {}
        token = strtok(NULL, "|");
    }
    
    printf("\n=== PLAYER STATS ===\n");
    printf("Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d\n", gold, weapon, damage, kills);
    
    if (strstr(weapon, "Dragon Claws") != NULL) {
        printf("Passive: 50%% Crit Chance\n");
    } else if (strstr(weapon, "Staff of Light") != NULL) {
        printf("Passive: 10%% Insta-Kill Chance\n");
    }
    
    if (in_battle) {
        printf("\nWARNING: Currently in battle!\n");
    }
}

void shop_menu() {
    char buffer[BUFFER_SIZE] = {0};
    send(sock, "SHOP", strlen("SHOP"), 0);
    int bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        printf("\nFailed to get shop items\n");
        return;
    }
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "SHOP|", 5) != 0) {
        printf("\nInvalid shop format received\n");
        return;
    }
    
    printf("\n== WEAPON SHOP ==\n");
    char* items = buffer + 5;
    char* item = strtok(items, "|");
    while (item != NULL) {
        printf("%s\n", item);
        item = strtok(NULL, "|");
    }
    
    printf("\nEnter weapon number to buy (0 to cancel): ");
    int choice;
    scanf("%d", &choice);
    clear_input_buffer();
    
    if (choice > 0 && choice <= 5) {
        char command[20];
        snprintf(command, sizeof(command), "BUY %d", choice);
        send(sock, command, strlen(command), 0);
        
        memset(buffer, 0, BUFFER_SIZE);
        bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            if (strncmp(buffer, "SUCCESS|", 8) == 0) {
                printf("\n%s\n", buffer + 8);
            } else if (strncmp(buffer, "ERROR|", 6) == 0) {
                printf("\nError: %s\n", buffer + 6);
            } else {
                printf("\nUnknown response: %s\n", buffer);
            }
        }
    }
}

void inventory_menu() {
    char buffer[BUFFER_SIZE] = {0};
    send(sock, "INVENTORY", strlen("INVENTORY"), 0);
    int bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        printf("\nFailed to get inventory\n");
        return;
    }
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "INVENTORY|", 10) != 0) {
        printf("\nInvalid inventory format received\n");
        return;
    }
    
    printf("\n== YOUR INVENTORY ==\n");
    char* items = buffer + 10;
    char* item = strtok(items, "|");
    while (item != NULL) {
        char* parts[4] = {0};
        int part_count = 0;
        
        // Split item into parts
        char* part = strtok(item, ":");
        while (part != NULL && part_count < 4) {
            parts[part_count++] = part;
            part = strtok(NULL, ":");
        }
        
        if (part_count > 0) {
            printf("[%s] %s", parts[0], parts[1]);
            if (part_count > 2) {
                printf(" (Passive: %s)", parts[2]);
            }
            if (part_count > 3 && strcmp(parts[3], "EQUIPPED") == 0) {
                printf(" [EQUIPPED]");
            }
            printf("\n");
        }
        
        item = strtok(NULL, "|");
    }
    
    printf("\nEnter weapon number to equip (0 for Fists): ");
    int choice;
    scanf("%d", &choice);
    clear_input_buffer();
    
    char command[20];
    snprintf(command, sizeof(command), "EQUIP %d", choice);
    send(sock, command, strlen(command), 0);
    
    memset(buffer, 0, BUFFER_SIZE);
    bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (strncmp(buffer, "SUCCESS|", 8) == 0) {
            printf("\n%s\n", buffer + 8);
        } else if (strncmp(buffer, "ERROR|", 6) == 0) {
            printf("\nError: %s\n", buffer + 6);
        } else {
            printf("\nUnknown response: %s\n", buffer);
        }
    }
}

void battle_mode() {
    char buffer[BUFFER_SIZE] = {0};
    
    // Start battle
    send(sock, "BATTLE", strlen("BATTLE"), 0);
    int bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        printf("\nFailed to start battle\n");
        return;
    }
    buffer[bytes_read] = '\0';
    
    if (strncmp(buffer, "BATTLE_START|", 13) != 0) {
        if (strncmp(buffer, "ERROR|", 6) == 0) {
            printf("\n%s\n", buffer + 6);
        } else {
            printf("\nInvalid battle start response: %s\n", buffer);
        }
        return;
    }
    
    // Parse battle info
    int enemy_hp = 0, reward = 0;
    char* token = strtok(buffer + 13, "|");
    while (token != NULL) {
        if (sscanf(token, "ENEMY_HP:%d", &enemy_hp) == 1) {}
        else if (sscanf(token, "REWARD:%d", &reward) == 1) {}
        token = strtok(NULL, "|");
    }
    
    printf("\n=== BATTLE STARTED ===\n");
    printf("Enemy HP: %d | Potential Reward: %d gold\n", enemy_hp, reward);
    
    // Battle loop
    while (1) {
        printf("\n> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "q") == 0) {
            send(sock, "FLEE", strlen("FLEE"), 0);
            bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                if (strncmp(buffer, "BATTLE_RESULT|FLED", 18) == 0) {
                    printf("\nYou fled from battle!\n");
                }
            }
            break;
        } 
        else if (strcmp(buffer, "attack") == 0) {
            char command[BUFFER_SIZE];
            snprintf(command, sizeof(command), "ATTACK ENEMY_HP:%d|REWARD:%d", enemy_hp, reward);
            send(sock, command, strlen(command), 0);
            
            memset(buffer, 0, BUFFER_SIZE);
            bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) {
                printf("\nBattle connection lost\n");
                break;
            }
            buffer[bytes_read] = '\0';
            
            if (strncmp(buffer, "BATTLE_UPDATE|", 14) == 0) {
                int damage = 0, critical = 0;
                token = strtok(buffer + 14, "|");
                while (token != NULL) {
                    if (sscanf(token, "DAMAGE:%d", &damage) == 1) {}
                    else if (sscanf(token, "CRITICAL:%d", &critical) == 1) {}
                    else if (sscanf(token, "ENEMY_HP:%d", &enemy_hp) == 1) {}
                    else if (sscanf(token, "REWARD:%d", &reward) == 1) {}
                    token = strtok(NULL, "|");
                }
                
                if (critical == 1) {
                    printf("\nCRITICAL HIT! You dealt %d damage!\n", damage);
                } else if (critical == 2) {
                    printf("\nINSTANT KILL! You dealt %d damage!\n", damage);
                } else {
                    printf("\nYou dealt %d damage!\n", damage);
                }
                
                printf("Enemy HP: %d\n", enemy_hp);
            } 
            else if (strncmp(buffer, "BATTLE_RESULT|VICTORY|", 22) == 0) {
                int damage = 0, critical = 0;
                token = strtok(buffer + 22, "|");
                while (token != NULL) {
                    if (sscanf(token, "DAMAGE:%d", &damage) == 1) {}
                    else if (sscanf(token, "CRITICAL:%d", &critical) == 1) {}
                    else if (sscanf(token, "REWARD:%d", &reward) == 1) {}
                    token = strtok(NULL, "|");
                }
                
                if (critical == 1) {
                    printf("\nCRITICAL HIT! You dealt %d damage!\n", damage);
                } else if (critical == 2) {
                    printf("\nINSTANT KILL! You dealt %d damage!\n", damage);
                } else {
                    printf("\nYou dealt %d damage!\n", damage);
                }
                
                printf("\n=== VICTORY ===\n");
                printf("You earned %d gold!\n", reward);
                break;
            }
        } 
        else {
            printf("Invalid command. Type 'attack' or 'exit'\n");
        }
    }
}

int main() {
    struct sockaddr_in serv_addr;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IP address
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/Address not supported\n");
        return -1;
    }
    
    // Connect to server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }
    
    printf("Connected to the dungeon server!\n");
    
    int choice;
    do {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clear_input_buffer();
        
        switch (choice) {
            case 1:
                show_stats();
                break;
            case 2:
                shop_menu();
                break;
            case 3:
                inventory_menu();
                break;
            case 4:
                battle_mode();
                break;
            case 5:
                send(sock, "EXIT", strlen("EXIT"), 0);
                printf("Exiting game...\n");
                break;
            default:
                printf("Invalid option. Please try again.\n");
        }
    } while (choice != 5);
    
    close(sock);
    return 0;
}
