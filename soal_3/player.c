#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>

#define IP "127.0.0.1"
#define PORT 8080

int main() {
    init_player();
    srand(time(NULL));

    // Setup koneksi ke server (dungeon.c)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    while(true) {
        int choice;
        printf("\n\n=== MAIN MENU ===");
        printf("\n1. Show Player Stats");
        printf("\n2. Shop (Buy Weapons)");
        printf("\n3. View Inventory & Equip Weapons");
        printf("\n4. Battle Mode");
        printf("\n5. Exit Game");
        printf("\nChoose an option: ");
        scanf("%d", &choice);

        switch(choice) {
            case 1: show_stats(); break;
            case 2: shop(); break;
            case 3: view_inventory(); break;
            case 4: battle_mode(); break;
            case 5: 
                close(sock);
                exit(0);
            default:
                printf("Invalid option. Please try again.");
        }
    }

    return 0;
}
