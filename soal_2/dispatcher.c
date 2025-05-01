#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_ORDERS 100
#define NAME_LEN 100
#define ADDR_LEN 100
#define TYPE_LEN 10
#define AGENT_LEN 50
#define LOGFILE "delivery.log"
#define CSVFILE "delivery_order.csv"

typedef struct {
    char name[NAME_LEN];
    char address[ADDR_LEN];
    char type[TYPE_LEN]; 
    bool delivered;
    char agent[AGENT_LEN]; 
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
} SharedData;

void download_csv() {
    printf("Downloading delivery_order.csv...\n");
    system("wget -q --show-progress --no-check-certificate -O delivery_order.csv 'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9'");
}

void load_orders_from_csv(SharedData *data) {
    FILE *file = fopen(CSVFILE, "r");
    if (!file) {
        perror("CSV file not found. Make sure download succeeded.");
        exit(1);
    }

    char line[300];
    fgets(line, sizeof(line), file); 

    while (fgets(line, sizeof(line), file)) {
        if (data->count >= MAX_ORDERS) break;
        char *name = strtok(line, ",");
        char *addr = strtok(NULL, ",");
        char *type = strtok(NULL, "\n");

        if (name && addr && type) {
            strncpy(data->orders[data->count].name, name, NAME_LEN);
            strncpy(data->orders[data->count].address, addr, ADDR_LEN);
            strncpy(data->orders[data->count].type, type, TYPE_LEN);
            data->orders[data->count].delivered = false;
            strcpy(data->orders[data->count].agent, "-");
            data->count++;
        }
    }

    fclose(file);
}

void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen(LOGFILE, "a");
    if (!log) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [AGENT %s] Reguler package delivered to [%s] in [%s]\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, name, address);

    fclose(log);
}

void deliver_order(SharedData *data, const char *target_name) {
    char *username = getenv("USER");
    if (!username) username = "unknown";

    for (int i = 0; i < data->count; i++) {
        if (!data->orders[i].delivered &&
            strcmp(data->orders[i].type, "Reguler") == 0 &&
            strcmp(data->orders[i].name, target_name) == 0) {

            data->orders[i].delivered = true;
            strncpy(data->orders[i].agent, username, AGENT_LEN);
            write_log(username, data->orders[i].name, data->orders[i].address);
            printf("Delivered order for [%s] to [%s]\n", data->orders[i].name, data->orders[i].address);
            return;
        }
    }

    printf("No pending Reguler order found for [%s].\n", target_name);
}

void print_all_orders(SharedData *data) {
    printf("All Orders:\n");
    for (int i = 0; i < data->count; i++) {
        const char *status = data->orders[i].delivered ? "Delivered" : "Pending";
        printf("%s - %s - %s - %s by %s\n", 
            data->orders[i].name, 
            data->orders[i].address, 
            data->orders[i].type,
            status,
            data->orders[i].agent);
    }
}

void print_order_status(SharedData *data, const char *name) {
    for (int i = 0; i < data->count; i++) {
        if (strcmp(data->orders[i].name, name) == 0) {
            if (data->orders[i].delivered) {
                printf("Status for %s: Delivered by Agent %s\n", name, data->orders[i].agent);
            } else {
                printf("Status for %s: Pending\n", name);
            }
            return;
        }
    }
    printf("No order found for %s.\n", name);
}

int main(int argc, char *argv[]) {
    key_t key = ftok("dispatcher.c", 123);
    int shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);

    if (data->count == 0) {
        download_csv();
        load_orders_from_csv(data);
    }

    if (argc == 2 && strcmp(argv[1], "-list") == 0) {
        print_all_orders(data);
    } else if (argc == 3 && strcmp(argv[1], "-deliver") == 0) {
        deliver_order(data, argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "-status") == 0) {
        print_order_status(data, argv[2]);
    } else {
        printf("Usage:\n");
        printf("  ./dispatcher -list\n");
        printf("  ./dispatcher -deliver [Name]\n");
        printf("  ./dispatcher -status [Name]\n");
    }

    shmdt(data);
    return 0;
}
