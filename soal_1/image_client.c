#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define SERVER_IP    "127.0.0.1"
#define SERVER_PORT  8080
#define BUFFER_SIZE  8192

void print_menu() {
    printf("\n=== Client Menu ===\n");
    printf("1. Send input file to server\n");
    printf("2. Download file from server\n");
    printf("3. Exit\n>> ");
}

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket error");
        return -1;
    }

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(SERVER_PORT),
        .sin_addr.s_addr = inet_addr(SERVER_IP)
    };

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return -1;
    }
    return sock;
}

void send_input_file() {
    char filename[256];
    printf("Enter the file name: ");
    scanf("%255s", filename);

    char path[512];
    snprintf(path, sizeof(path), "client/secrets/%s", filename);

    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Failed to open file");
        return;
    }

    char *file_data = malloc(BUFFER_SIZE);
    size_t read_size = fread(file_data, 1, BUFFER_SIZE - 1, fp);
    file_data[read_size] = '\0';
    fclose(fp);

    int sock = connect_to_server();
    if (sock < 0) {
        free(file_data);
        return;
    }

    char *msg = malloc(read_size + 512);
    int len = snprintf(msg, read_size + 512,
                       "DECRYPT:%s\n%s\n\n",
                       filename, file_data);

    send(sock, msg, len, 0);
    shutdown(sock, SHUT_WR);

    char response[BUFFER_SIZE];
    int n = recv(sock, response, sizeof(response)-1, 0);
    if (n > 0) {
        response[n] = '\0';
        if (strncmp(response, "OK:", 3) == 0) {
            printf("Server: Text decrypted and saved as %s", response + 3);
        } else {
            printf("Server Error: %s", response + 4);
        }
    } else {
        printf("No response from server.\n");
    }

    close(sock);
    free(file_data);
    free(msg);
}

void download_file() {
    char filename[256];
    printf("Enter the file name to download: ");
    scanf("%255s", filename);

    int sock = connect_to_server();
    if (sock < 0) return;

    char request[512];
    int rlen = snprintf(request, sizeof(request), "DOWNLOAD:%s\n", filename);
    send(sock, request, rlen, 0);
    shutdown(sock, SHUT_WR);

    char header[BUFFER_SIZE];
    int n = recv(sock, header, sizeof(header)-1, 0);
    if (n <= 0) {
        printf("No response from server.\n");
        close(sock);
        return;
    }
    header[n] = '\0';
    if (strncmp(header, "OK:", 3) != 0) {
        printf("Server Error: %s", header + 4);
        close(sock);
        return;
    }

    // simpan file langsung di folder client
    char save_path[512];
    snprintf(save_path, sizeof(save_path), "client/%s", filename);
    FILE *fp = fopen(save_path, "wb");
    if (!fp) {
        perror("File write error");
        close(sock);
        return;
    }

    char *body = strchr(header, '\n');
    if (body) {
        body++;
        int body_len = n - (body - header);
        fwrite(body, 1, body_len, fp);
    }

    while ((n = recv(sock, header, sizeof(header), 0)) > 0) {
        fwrite(header, 1, n, fp);
    }

    fclose(fp);
    close(sock);
    printf("Success! Image saved as %s\n", save_path);
}

int main() {
    while (1) {
        print_menu();
        int choice;
        if (scanf("%d", &choice) != 1) break;
        getchar();

        switch (choice) {
            case 1: send_input_file(); break;
            case 2: download_file();   break;
            case 3: return 0;
            default: printf("Invalid choice.\n"); break;
        }
    }
    return 0;
}
