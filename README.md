# Sisop-3-2025-IT26
Anggota Kelompok:<br />
Azaria Raissa Maulidinnisa 5027241043<br />
Oscaryavat Viryavan 5027241053<br />
Naufal Ardhana 5027241118<br />

## Soal no 1

## Soal no 2
### Dispatcher.c
```
typedef struct {
    char name[NAME_LEN];
    char address[ADDR_LEN];
    char type[TYPE_LEN];       // Express / Reguler
    bool delivered;            // status pengiriman
    char agent[AGENT_LEN];     // nama agen pengantar
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;                // total order yang terbaca dari CSV
} SharedData;
```
`char` diatas merupakan beberaoa structc yang berisi diantaranya ialah tipe Ekspres/reguler, status pengiriman, serta nama agen pengantar. Sedangkan struct untuk `SharedData` akan menyimpan semua pesanan dalam 1 blok shared memory antara dispatcher dan delivery agent.
```
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
```
Fungsi `while` akan membuka file CSV dan melewati baris pertama kemudian membacam isis baris demi baris. Fungsi ini juga akan memprasing isi CSV dan menyimpannya ke array shared memory. Semua order akan dilabel sebagai belum dikirm (`delivered = false`) dan `agent = "-"`.

```
void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen(LOGFILE, "a");
    ...
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
```
Entri akan ditulis catatan pengiriman ke `delivery.log` yang nantinya akan bisa dicek menggunakan `cat`. Entri akan memiliki format waktu realtime sesuai yang diminta oleh soal.

```
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
```
Fungsi `deliver_order` akan mengambil nama user (agen) dari lingkungan variabel `USER` yang nantinya akan digunakan untuk pengiriman reguler, pada command `./dispatcher -deliver [Nama]`. Fungsi juga akan menelusuri pesanan reguler yang belum dikirim dan cocok dengan nama target. Pada `data->orders` fungsi akan menandai apabila memang produk terkirim dan akan dicatat ke dalam `deliveri.log`.

```
void print_all_orders(SharedData *data) {
    ...
    printf("%s - %s - %s - %s by %s\n", ...
}
void print_order_status(SharedData *data, const char *name) {
    ...
}

```
Pada fungsi `print_all_orders` ia akan mencetak seluruh pesanan dalam shared memory `delivery.log` yang menampilkan nama, alamat, tipe, status, dan agen yang bertugas mengirimkan. Kemudian ada fungsi `print_order_status` yang akan mencari order berdasarkan nama. JIka ditemukan status akan ditampilan delivered atau pending.

```
key_t key = ftok("dispatcher.c", 123);
int shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
if (data->count == 0) {
    download_csv();
    load_orders_from_csv(data);
}
if (argc == 2 && strcmp(argv[1], "-list") == 0) { ... }
else if (argc == 3 && strcmp(argv[1], "-deliver") == 0) { ... }
else if (argc == 3 && strcmp(argv[1], "-status") == 0) { ... }


```
Fungsi utama atau `main` akan membuat key unik untuk shared memory. Dengan `shmget` untuk membuat atau memngambil shared memory dan `shmat` untuk mengaitkan segmen shared memory ke pointer data. Jika belum ada data, maka unduh ia akan mendownload CSV dan memparsing datanya. Kemudian selanjutnya ada command utama untuk mengcompile isi dispatcher yaitu `-list`, `-deliver`, dan `-status [Nama]` dengan fungsi masing masing berdasarkan soal.

### Delivery_agent.c
```
void write_log(const char *agent, const char *name, const char *address) {
    FILE *log = fopen(LOGFILE, "a"); 
    if (!log) {
        perror("Failed to open log file"); 
        return;
    }

    time_t now = time(NULL);             
    struct tm *t = localtime(&now);
fprintf(log, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to [%s] in [%s]\n",
            t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
            t->tm_hour, t->tm_min, t->tm_sec,
            agent, name, address);
```
Fungsi `wite_log` akan menulis list pengiriman di `delivery.log` secara append. Ia akan menulis secara real time dan akan mencetak file log seperti format yang diminta oleh soal.

```
void *agent_function(void *arg) {
    char *agent_name = (char *)arg; 
    key_t key = ftok("dispatcher.c", 123);
    int shmid = shmget(key, sizeof(SharedData), 0666);
    if (shmid < 0) {
        perror("Gagal mendapatkan shared memory"); // Jika gagal, keluar dari thread
        pthread_exit(NULL);
    }
    SharedData *data = (SharedData *)shmat(shmid, NULL, 0);
```
Ini merupakan Fungsi thread utama yang dijalan oleh setiap agent dimana pada `key-t` program akan membuat kunci untuk shared memory  berdasarkan file `dispatcher.c` dan ID 123. . Di `int shmid` program akan mengambil ID dengan ukuran struktur shared data. Kemudian akan dibawah shared memory ke memori loka user.

```
while (1) {
        for (int i = 0; i < data->count; i++) {
            if (!data->orders[i].delivered && strcmp(data->orders[i].type, "Express") == 0) {
                data->orders[i].delivered = true; 
                strncpy(data->orders[i].agent, agent_name, AGENT_LEN);
                write_log(agent_name, data->orders[i].name, data->orders[i].address);

                printf("%s delivered order for %s\n", agent_name, data->orders[i].name);
                sleep(1);
            }
        }
        sleep(2);
    }
```
Loop tanpa akhir dimana agen akan terus bekerja memeriksa pesanan baru di file csv. `if (!data->orders[i]` akan mengecek apakah order bertipe Express dan belum dikirm. Kemudian fungsi `strncpy` akan mencatat siapa agen yang mengirim dan nantinya akan ditulis ke dalam file `delivery.log`. `printf` dimana akanmenampilkan info pengiriman dengan delay 1 detik setelah pengiriman. Delay 2 detik dibutuhkan sebelum memeriksa ulang agar tidak boros CPU.

```
int main() {
    pthread_t agents[3];  
    const char *names[] = {"AGENT A", "AGENT B", "AGENT C"}; 
    for (int i = 0; i < 3; i++) {
        pthread_create(&agents[i], NULL, agent_function, (void *)names[i]);
    }

        for (int i = 0; i < 3; i++) {
        pthread_join(agents[i], NULL);
    }

    return 0;
```
Fungsi utama dimana `pthread` merupakan array untuk menyimpan thread 3 agen tadi. `for (int i = 0; i < 3; i++)` akan membuat 3 thread masing masing untuk 1 agen yang bertugas sesuai instruksi soal. Kemudia tunggu semua thread karena loop infinite terjadi di `agent_function`. Shared memory digunakan agar sinkronasi data ke-2 proses terjadi secara real time.



## Soal no 3

## Soal no 4

`#include "shm_common.h"` digunakan di system.c dan hunter.c agar keduanya memiliki akses terhadap clue (struktur data dan key yang sama dalam shared memory).

```
// hunter.c
int shmid = shmget(key, sizeof(SystemData), 0666);
    
    if (shmid == -1) {
        printf("System is not running!\n");
        return 1;
    }
```
Fungsi ini mencegah user menjalankan hunter sebelum system dijalankan. 
<br>
<img src = "https://github.com/user-attachments/assets/4eba56ab-48e7-460a-923b-f94fa35c3231" width = "400"> <br>
```
// system.c > generate_dungeon(SystemData* data)
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

d->min_level = rand() % 5 + 1;   // Level antara 1-5
    d->exp = rand() % 151 + 150;    // ATK antara 150-300
    d->atk = rand() % 51 + 100;     // HP antara 100-150
    d->hp = rand() % 51 + 50;       // DEF antara 50-100
    d->def = rand() % 26 + 25;      // EXP antara 25-50
    d->shm_key = ftok("/tmp", 200 + data->num_dungeons);
```
Fungsi `generate_dungeon` pada system.c berfungsi untuk membuat dungeon baru secara acak dengan nama yang sudah tertera dan memberikan stats dan minimum secara acak dengan constrain tertentu, lalu
menambahkannya ke dalam shared memory yang dikelola oleh SystemData.
```
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
```

Fungsi ini menyetak dungeon yang telah di-generate di system, namun tidak semua dungeon yang telah digenerate oleh system.c dapat dilihat semua user, dungeon yang dapat dilihat bergantung dengan level user dan minimum level dungeonnya, semakin tinggi level user, semakin banyak pula dungeon yang bisa dilihat dan bisa digunakan.
