#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_ORDERS 100
#define NAME_LEN 100
#define ADDR_LEN 100
#define TYPE_LEN 10
#define AGENT_LEN 50
#define LOGFILE "delivery.log"

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

SharedData *shared_data;

void tulis_log(const char *agent, const char *nama, const char *alamat) {
    FILE *log = fopen(LOGFILE, "a");
    if (!log) {
        perror("Gagal membuka delivery.log");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to [%s] in [%s]\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, nama, alamat);

    fclose(log);
}

void* agen_thread(void *arg) {
    char *agen_nama = (char *)arg;

    while (1) {
        for (int i = 0; i < shared_data->count; i++) {
            if (!shared_data->orders[i].delivered &&
                strcmp(shared_data->orders[i].type, "Express") == 0) {
                
                // Tandai dikirim
                shared_data->orders[i].delivered = true;
                strncpy(shared_data->orders[i].agent, agen_nama, AGENT_LEN);

                tulis_log(agen_nama, shared_data->orders[i].name, shared_data->orders[i].address);

                sleep(1); 
            }
        }

        sleep(2); 
    }

    return NULL;
}

int main() {
    key_t key = ftok("dispatcher.c", 123);
    int shmid = shmget(key, sizeof(SharedData), 0666);
    if (shmid == -1) {
        perror("Gagal mendapatkan shared memory");
        exit(1);
    }

    shared_data = (SharedData *)shmat(shmid, NULL, 0);
    if (shared_data == (void *) -1) {
        perror("Gagal menempelkan shared memory");
        exit(1);
    }

    pthread_t thread_a, thread_b, thread_c;
    pthread_create(&thread_a, NULL, agen_thread, "AGENT A");
    pthread_create(&thread_b, NULL, agen_thread, "AGENT B");
    pthread_create(&thread_c, NULL, agen_thread, "AGENT C");

    pthread_join(thread_a, NULL);
    pthread_join(thread_b, NULL);
    pthread_join(thread_c, NULL);

    shmdt(shared_data);
    return 0;
}
