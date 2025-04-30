#include "shm_common.h"


int main() {
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(SystemData), 0666);
    if (shmid == -1) {
        printf("Shared memory belum tersedia!\n");
        return 1;
    }

    SystemData* data = shmat(shmid, NULL, 0);
    char username[50];
    printf("Masukkan username: ");
    scanf("%s", username);

    int idx = -1;
    for (int i = 0; i < data->num_hunters; ++i) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        printf("Hunter tidak ditemukan!\n");
        return 1;
    }

    struct Hunter* h = &data->hunters[idx];
    if (h->banned) {
        printf("Anda dibanned.\n");
        return 1;
    }

    printf("Selamat datang %s (Lv:%d)\n", h->username, h->level);

    while (1) {
        int pilih;
        puts("\n1. Info Hunter\n2. Masuk Dungeon\n3. Logout");
        printf("Pilih: "); scanf("%d", &pilih);

        if (pilih == 1) {
            printf("LV:%d EXP:%d HP:%d ATK:%d DEF:%d\n",
                h->level, h->exp, h->hp, h->atk, h->def);
        }
        else if (pilih == 2) {
            printf("Pilih dungeon:\n");
            for (int i = 0; i < data->num_dungeons; ++i)
                printf("[%d] %s (MinLv:%d)\n", i+1, data->dungeons[i].name, data->dungeons[i].min_level);

            int pilih_dg; scanf("%d", &pilih_dg); pilih_dg--;
            if (pilih_dg < 0 || pilih_dg >= data->num_dungeons) continue;

            Dungeon* d = &data->dungeons[pilih_dg];
            if (h->level < d->min_level) {
                printf("Level terlalu rendah.\n");
                continue;
            }
            printf("Bertarung melawan %s...\n", d->name);

            // Simulasi battle
            h->exp += d->exp;
            h->hp -= (d->atk - h->def);
            if (h->hp <= 0) {
                printf("Hunter mati!\n");
                h->hp = 1;
            } else {
                printf("Menang! EXP +%d\n", d->exp);
            }
            char notif[200];
            sprintf(notif, "%s menyelesaikan %s", h->username, d->name);
            strncpy(data->notif[data->current_notification_index], notif, 99);
            data->notif[data->current_notification_index][99] = '\0';
            data->current_notification_index = (data->current_notification_index + 1) % MAX_NOTIF;
        }
        else break;
    }
    shmdt(data);
    return 0;
}
