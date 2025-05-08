# Sisop-3-2025-IT26

| Nama                         | Nrp        |
| ---------------------------- | ---------- |
| Azaria Raissa Maulidinnisa   | 5027241043 |
| Oscaryavat Viryavan          | 5027241053 |
| Naufal Ardhana               | 5027241118 |

## Soal no 1
Mendownload file zip menggunakan `wget`, `wget -O secrets.zip https.......`.
Unzip file zip tersebut ke dalam direktori client, `unzip secrets.zip -d client`.
Hapus file zip yang sudah di unzip sebelumnya menggunakan `unlink secrets.zip`.

### image_client.c
```
void print_menu() {
    printf("\n=== Client Menu ===\n");
    printf("1. Send input file to server\n");
    printf("2. Download file from server\n");
    printf("3. Exit\n>> ");
}
```
Fungsi `print_menu` menampilkan pilihan menu utama ke pengguna di terminal. Menu berisi tiga opsi: mengirim file input ke server, mengunduh file dari server, dan keluar dari program.  
```
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
```
Fungsi `connect_to_server` bertugas membuat koneksi TCP ke server. Langkah-langkahnya membuat socket dengan protokol TCP `SOCK_STREAM`, kemudian menyiapkan alamat server dalam sturktur `sockaddr_in`, kemudian fungsi melakukan koneksi ke server menggunakan `connect()`. Jika berhasil, fungsi mengembalikan descriptor socket. Jika gagal, fungsi mencetak pesan error dan mengembalikan -1. 
```
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
```
User diminta memasukkan nama file yang ingin dikirim. Nama file ini akan digunakan untuk membentuk path file. Membangun path lengkap ke file di folder `client/secrets/`, misalnya jika user mengetik input_1.txt, maka path-nya menjadi client/secrets/input_1.txt. File dibuka dalam mode baca `r`. Jika gagal, program mencetak pesan error dan keluar dari fungsi. Membaca isi file ke buffer dengan Mengalokasikan buffer untuk isi file. Membaca maksimal `BUFFER_SIZE - 1` byte ke `file_data`. Menambahkan null-terminator (`\0`) di akhir supaya bisa diproses sebagai string. Kemudian, file ditutup setelah dibaca. Fungsi `connect_to_server()` dipanggil untuk membuat koneksi TCP ke server. Jika gagal, memori dibebaskan dan fungsi keluar. Menyiapkan dan mengirim pesan ke server dengan Format pesan: `DECRYPT:<nama_file>\n<isi_file>\n\n`. Pesan dikirim ke server menggunakan `send()`. Setelah pengiriman selesai, koneksi disinyalir berakhir dengan `shutdown()` arah penulisan. Menerima dan memproses balasan dari server. Bila ada data, tambahkan null-terminator untuk mengubahnya jadi string. Jika server mengirim balasan dengan awalan `"OK:"`, artinya dekripsi sukses. Jika bukan, pesan error dari server ditampilkan.
Jika tidak ada respons sama sekali, tampilkan peringatan. Terakhir, menutup koneksi socket dan membebaskan memori yang dialokasikan sebelumnya.
```
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
```
User diminta mengetik nama file yang ingin diunduh. Nama ini akan dikirim ke server. Membuka koneksi ke server dengan memanggil fungsi `connect_to_server()` untuk membuat koneksi socket TCP. Jika gagal, keluar dari fungsi. Menyusun dan mengirim request ke server dengan Menyusun permintaan dalam format `DOWNLOAD:<nama_file>\n`. Dikirim melalui socket ke server. Lalu dilakukan `shutdown()` pada arah tulis sebagai sinyal bahwa klien selesai mengirim. Menerima balasan pertama dari server. Jika tidak ada data, ditampilkan pesan bahwa server tidak merespons. Tambahkan null-terminator agar bisa diproses sebagai string. Jika respons tidak diawali dengan `"OK:"`, maka server mengirim error (misalnya file tidak ditemukan). Program menampilkan error tersebut dan keluar. Membuat file tujuan di folder `client/` dengan Path lengkap file hasil unduhan dibentuk, misalnya `client/image.jpg`. File dibuka (atau dibuat) untuk penulisan biner (wb). Jika gagal membuat file, program keluar dengan pesan error. Mencari awal isi file pada respons pertama (setelah baris header). Data yang sudah diterima sebagian ini langsung ditulis ke file. Melakukan `recv()` berulang kali untuk menerima potongan data file selanjutnya dari server.
Data ditulis langsung ke file tanpa diproses. File ditutup setelah selesai ditulis. Socket ditutup karena komunikasi selesai. Menampilkan konfirmasi bahwa file berhasil disimpan.

### image_server.c
`#define PORT   8080` 
Baris ini mendefinisikan sebuah makro bernama PORT dengan nilai 8080.
Makro ini biasanya digunakan untuk menetapkan port TCP yang akan dipakai oleh server untuk mendengarkan koneksi dari client.
`#define CHUNK_SIZE 4096`
Makro `CHUNK_SIZE` memiliki nilai 4096 (4 KB).
Nilai ini umumnya digunakan sebagai ukuran buffer saat membaca atau menulis data dalam potongan (chunk).
`#define LOG_PATH "server/server.log"`
Makro LOG_PATH menyimpan path ke file log server, yaitu "`server/server.log`".
File log ini digunakan untuk mencatat: Aktivitas server (seperti koneksi yang masuk), perintah dari client, error yang terjadi.
`#define DB_DIR "server/database/"`
Makro `DB_DIR` menunjukkan folder tempat penyimpanan file hasil (output) server, seperti file hasil dekripsi, download, dll.
```
void log_action(const char* src, const char* act, const char* info) {
    FILE* log = fopen(LOG_PATH, "a");
    if (!log) return;
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(log,
      "[%s][%04d-%02d-%02d %02d:%02d:%02d]: [%s] [%s]\n",
      src,
      tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
      tm.tm_hour, tm.tm_min, tm.tm_sec,
      act, info
    );
    fclose(log);
}
```
Fungsi log_action digunakan untuk mencatat aktivitas tertentu ke dalam file log server. Parameter yang digunakan: `src`: sumber aksi, misalnya "CLIENT", "SERVER", "AUTH", dll. `act`: aksi atau perintah yang dilakukan, misalnya "`DECRYPT`", "`DOWNLOAD`", dll. `info`: detail tambahan, seperti nama file atau hasil operasi. `fopen(LOG_PATH, "a")`: membuka file log (misalnya `server/server.log`) dalam mode append (`"a"`), artinya data baru akan ditambahkan di akhir file tanpa menghapus data sebelumnya. Jika file tidak bisa dibuka (misalnya karena tidak ada izin atau path salah), fungsi langsung keluar tanpa mencatat apa pun. `time(NULL)` mengambil waktu saat ini dalam format epoch. `localtime()` mengonversinya ke struktur `tm` yang berisi komponen waktu lokal (tahun, bulan, hari, jam, menit, detik). Menuliskan satu baris log dengan format sebagai berikut: `[SOURCE][YYYY-MM-DD HH:MM:SS]: [ACTION] [INFO]`. Setelah mencatat log, file ditutup untuk menyimpan perubahan dan mencegah kebocoran file descriptor. Fungsi `log_action()` berguna untuk: Melacak semua aktivitas yang dilakukan oleh server, memberikan informasi waktu dan konteks dari setiap tindakan, membantu proses debugging dan audit sistem.
```
char* reverse_str(const char* s) {
    int n = strlen(s);
    char* r = malloc(n+1);
    for (int i = 0; i < n; i++) r[i] = s[n-1-i];
    r[n] = '\0';
    return r;
}
```
Fungsi ini menerima input berupa string (`const char* s`) dan mengembalikan string baru yang merupakan hasil pembalikan dari string input. Menggunakan fungsi `strlen` untuk mengetahui panjang dari string `s`. Mengalokasikan memori sebanyak `n+1` karakter untuk menyimpan hasil pembalikan string. Tambahan `+1` dibutuhkan untuk menampung karakter null-terminator `\0` di akhir string. Melakukan iterasi sebanyak `n` kali. Setiap karakter dari posisi akhir di `s` dipindahkan ke awal di `r`. Menambahkan karakter `'\0'` di akhir string agar string valid dan bisa dikenali sebagai akhir oleh fungsi-fungsi C standar. Mengembalikan pointer ke string hasil pembalikan. Fungsi ini tidak mengubah string asli karena `s` bertipe `const char*`. 
```
char* hex_decode(const char* hex, int* out_len) {
    int len = strlen(hex);
    if (len % 2 != 0) return NULL;
    *out_len = len/2;
    char* out = malloc(*out_len);
    for (int i = 0; i < *out_len; i++) {
        unsigned int byte;
        if (sscanf(hex + 2*i, "%2x", &byte) != 1) {
            free(out);
            return NULL;
        }
        out[i] = (char)byte;
    }
    return out;
}
```
Fungsi `hex_decode`, yang digunakan untuk mengubah (decode) string hexadecimal menjadi data asli (byte array atau string biner). Panjang string heksadesimal harus genap (karena setiap byte diwakili oleh 2 digit heksadesimal). Jika tidak genap, tidak valid → `return NULL`. Setiap 2 karakter hex menghasilkan 1 byte → hasil decode akan memiliki panjang `len / 2`. Alokasi memori sebanyak `out_len` untuk menyimpan hasil decode. Iterasi setiap 2 karakter `(%2x)` dari input hex, lalu diubah menjadi byte (unsigned int byte). Fungsi `sscanf(hex + 2*i, "%2x", &byte)` membaca 2 karakter hex dan mengubahnya jadi angka (0–255).
Jika gagal decode, maka: `free(out)` → bebaskan memori dan `return NULL`. Setiap byte hasilnya disimpan ke `array out[i]`. Setelah semua byte berhasil dikonversi, hasilnya dikembalikan.
```
void decrypt_and_save(int client, const char* filename, const char* rawdata) {
    log_action("Client", "DECRYPT", filename);

    // filter hanya hex digit
    int L = strlen(rawdata);
    char* hex = malloc(L+1);
    int h = 0;
    for (int i = 0; i < L; i++) {
        if (isxdigit((unsigned char)rawdata[i])) {
            hex[h++] = rawdata[i];
        }
    }
    hex[h] = '\0';

    if (h == 0 || (h % 2) != 0) {
        send(client, "ERR:Invalid hex input\n", 21, 0);
        free(hex);
        return;
    }

    // reverse & decode
    char* rev = reverse_str(hex);
    free(hex);
    int bin_len;
    char* bin = hex_decode(rev, &bin_len);
    free(rev);
    if (!bin) {
        send(client, "ERR:Invalid hex input\n", 21, 0);
        return;
    }

    // buat filename berdasarkan timestamp
    char outname[64];
    time_t now = time(NULL);
    snprintf(outname, sizeof(outname), "%ld.jpeg", now);

    // simpan file
    char path[128];
    snprintf(path, sizeof(path), DB_DIR "%s", outname);
    FILE* f = fopen(path, "wb");
    if (!f) {
        send(client, "ERR:Failed to write file\n", 25, 0);
        free(bin);
        return;
    }
    // tulis seluruh data binari
    if (fwrite(bin, 1, bin_len, f) != (size_t)bin_len) {
        send(client, "ERR:Write incomplete\n", 20, 0);
        fclose(f);
        free(bin);
        return;
    }
    fclose(f);
    free(bin);

    log_action("Server", "SAVE", outname);

    char resp[80];
    snprintf(resp, sizeof(resp), "OK:%s\n", outname);
    send(client, resp, strlen(resp), 0);
}
```
Fungsi `decrypt_and_save` mencatat bahwa client melakukan permintaan dekripsi terhadap `filename`. Menyaring `rawdata`, menyisakan hanya karakter yang valid sebagai hex digit (`0–9, a–f, A–F`). Disimpan ke variabel baru hex. Jika jumlah karakter hex tidak genap (karena 1 byte = 2 hex) atau kosong, kirim error ke client dan akhiri proses. Balik string hex (`reverse_str`) karena client mengirim dalam urutan terbalik. Decode dari hex → byte array (`hex_decode`). Jika gagal, kirim pesan error ke client. Membuat nama file unik berdasarkan timestamp UNIX (misal `1715151201.jpeg`) untuk menghindari duplikat. File disimpan sebagai binary (`wb`) di folder `server/database/`. Tulis semua byte hasil decode ke file. Jika jumlah byte yang ditulis tidak sesuai, kirim error dan keluar. Tutup file dan bebaskan memori. Log bahwa server telah menyimpan file hasil dekripsi. Memberi tahu client bahwa proses dekripsi dan penyimpanan berhasil, serta nama file hasilnya.
```
void send_file(int client, const char* filename) {
    log_action("Client", "DOWNLOAD", filename);
    char path[128];
    snprintf(path, sizeof(path), DB_DIR "%s", filename);
    FILE* f = fopen(path, "rb");
    if (!f) {
        send(client, "ERR:File not found\n", 19, 0);
        return;
    }
    char hdr[80];
    snprintf(hdr, sizeof(hdr), "OK:%s\n", filename);
    send(client, hdr, strlen(hdr), 0);

    char buf[CHUNK_SIZE];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        send(client, buf, n, 0);
    }
    fclose(f);
    log_action("Server", "UPLOAD", filename);
}
```
Fungsi `send_file` Mencatat di file log bahwa client meminta file tertentu untuk di-download. Menyusun path lengkap file (misal: `server/database/1715151201.jpeg`). Membuka file dalam mode read binary (`rb`). Jika gagal (file tidak ditemukan), kirim error ke client dan keluar. Mengirim header ke client berisi status berhasil dan nama file, misalnya: `OK:1715151201.jpeg`. Membaca isi file per `CHUNK_SIZE` byte (4096 byte per iterasi). Mengirim tiap potongan (`buf`) ke client hingga file habis terbaca. Menutup file dan mencatat bahwa server telah mengirim file ke client (aksi "UPLOAD"). 
```
void handle_client(int client) {
    // buffer dinamis
    size_t cap = CHUNK_SIZE, len = 0;
    char* buf = malloc(cap);
    char tmp[CHUNK_SIZE];
    ssize_t n;
    // baca hingga client shutdown write
    while ((n = recv(client, tmp, sizeof(tmp), 0)) > 0) {
        if (len + n >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        memcpy(buf + len, tmp, n);
        len += n;
    }
    if (len == 0) { free(buf); close(client); return; }
    buf[len] = '\0';

    // pisah header/body pada newline pertama
    char* p = memchr(buf, '\n', len);
    if (!p) {
        send(client, "ERR:Invalid format\n", 19, 0);
        free(buf);
        close(client);
        return;
    }
    *p = '\0';
    const char* header = buf;
    const char* body   = p+1;

    if (strncmp(header, "DECRYPT:", 8) == 0) {
        decrypt_and_save(client, header+8, body);
    }
    else if (strncmp(header, "DOWNLOAD:", 9) == 0) {
        send_file(client, header+9);
    }
    else {
        send(client, "ERR:Unknown command\n", 20, 0);
    }

    free(buf);
    close(client);
}
```
 Fungsi `handle_client` Inisialisasi Buffer Dinamis, `cap`: kapasitas awal buffer (4096 byte), `len`: panjang data yang telah diterima sejauh ini, `buf`: buffer utama yang akan menyimpan seluruh data, `tmp`: buffer sementara untuk menerima data per chunk. Menerima data dari client menggunakan `recv`. Jika buffer utama tidak cukup besar, perbesar kapasitas dua kali lipat (`realloc`). Gabungkan data dari `tmp` ke `buf`. Jika tidak ada data yang masuk, langsung tutup koneksi. Tambahkan null terminator agar `buf` bisa diperlakukan sebagai string. Mencari newline pertama (`\n`) sebagai pemisah antara header dan body. Jika tidak ditemukan, kirim error dan keluar. `header`: bagian awal dari `buf` (sebelum \n), berisi perintah seperti `DECRYPT:filename`. `body`: bagian setelah newline, berisi konten data (misal: teks hex terenkripsi). Mengecek perintah di bagian `header`. Jika `DECRYPT:<filename>`, maka jalankan fungsi dekripsi dan simpan file. Jika `DOWNLOAD:<filename>`, kirim file ke client. Jika tidak dikenal, kirim error.
Hapus buffer yang telah dialokasikan. Tutup koneksi socket client.

### output client menu
![Screenshot 2025-05-08 082951](https://github.com/user-attachments/assets/b1131d2c-c13d-4e29-9b5b-2afca8d7e5c1)
![Screenshot 2025-05-08 083325](https://github.com/user-attachments/assets/306e5c01-8110-451c-8fd2-cdfadf84e1f6)

### isi server.log
![Screenshot 2025-05-08 083457](https://github.com/user-attachments/assets/5ccdda3b-70ae-4c16-8fc3-1eb1fc3eeb84)

### struktur direktori akhir 
![Screenshot 2025-05-08 083551](https://github.com/user-attachments/assets/55e0b154-89f7-4fc0-b72f-d0e93e182f57)

### rootkids
![Screenshot 2025-05-08 083713](https://github.com/user-attachments/assets/221cff17-51f4-4fb3-ad86-48f5fc5bcc00)

### Revisi 
```
    //Simpan direktori kerja saat ini
    char old_cwd[PATH_MAX];
    if (!getcwd(old_cwd, sizeof(old_cwd))) {
        perror("getcwd failed");
        exit(1);
    }

    // Fork proses utama untuk membuat daemon
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }
    if (pid > 0) {
        // Proses induk mengakhiri dirinya sendiri
        exit(0);
    }

    // Set session ID dan detasemen dari terminal
    if (setsid() < 0) {
        perror("setsid failed");
        exit(1);
    }

    // Kembalikan direktori kerja ke aplikasi
    if (chdir(old_cwd) < 0) {
        perror("chdir failed");
        // Lanjutkan meskipun terjadi kesalahan pada chdir
    }

    // Tutup file descriptor standar
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
```
Mengubah proses biasa menjadi daemon, yaitu proses latar belakang (background service) yang berjalan tanpa terikat terminal/logged-in user. `getcwd()` digunakan untuk menyimpan direktori kerja saat ini ke variabel `old_cwd`. Hal ini penting jika ingin nanti kembali ke direktori semula setelah daemonisasi. Jika gagal, program berhenti. `fork()` digunakan untuk membuat proses baru (anak). Proses induk langsung keluar, sehingga hanya proses anak yang lanjut dan menjadi daemon. Ini adalah langkah standar dalam pembuatan daemon agar tidak menjadi proses grup leader. `setsid()` membuat session baru, menjadikan proses sebagai session leader. Ini juga melepaskan proses dari terminal yang sebelumnya melekat. Ini mencegah proses menerima sinyal dari terminal. `chdir()` digunakan untuk kembali ke direktori kerja semula. Biasanya daemon pindah ke `/`, tapi dalam kasus ini ingin tetap di direktori semula. Jika gagal, hanya akan menampilkan error tapi program tetap jalan. Menutup `stdin, stdout, dan stderr` untuk benar-benar melepaskan daemon dari terminal. Proses daemon seharusnya tidak bergantung pada terminal input/output. Output log biasanya dialihkan ke file (misalnya `server.log`).

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

### Output ./dispatcher -list
![image](https://github.com/user-attachments/assets/3a483731-7e4d-41fb-82fe-4c848c0643f6)
### Ouput ./dispatcher -deliver
![image](https://github.com/user-attachments/assets/da2b4b86-9d2d-48f5-87ab-f5e2db1fbe24)
### Output ./dispatcher -status
![image](https://github.com/user-attachments/assets/8b36f006-7ce0-481f-8826-59af9fab7ca7)
### cat delivery.log
![image](https://github.com/user-attachments/assets/fe1fc50c-4024-418e-8595-1e97ce4f2b71)
Dapat dilihat jika log menampilan sesuai dengan pengiriman oleh delivery agent dan khusus untuk `./dispatcher -deliver` mengirimkan opsi reguler dengan nama user yang tertera.

## Soal no 3

Cara compiile soal nomor 3
```
gcc dungeon.c shop.c -o dungeon -lpthread
```

```
gcc -o player player.c
```
Setelah compile, lalu jalankan dengan `./dungeon` terlebih dahulu sebelum `./player` lalu Dungeon akan running di port 8080 seperti pada gambar dan abaikan pesan di kanan bawah.  <br>

<img src = "https://github.com/user-attachments/assets/5876c801-4ff5-408f-b5de-a05e8378c030" width = "400"> <br>

Program tidak bisa dijalankan kalau user command `./player` sebelum `./dungeon` <br>

<img src = "https://github.com/user-attachments/assets/3b6cc993-afd4-4670-9328-b7c68295edd8" width = "400"> <br>

Setelah itu akan tampil beberapa menu seperti pada gambar di bawah <br>

<img src = "https://github.com/user-attachments/assets/0a184f4e-e3b7-4ba8-a71b-925ff27cc7dc" width = "400"> <br>

`printf("Gold: %d | Equipped Weapon: %s | Base Damage: %d | Kills: %d\n", gold, weapon, damage, kills);`
Kode ini memuat informasi seputar default stats yang dimiliki user awal permainan.

<img src = "https://github.com/user-attachments/assets/7ed2b6f6-bf0a-442f-9e39-de1c77733bb7" width = "400"> <br>


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
Fungsi di atas ini mencegah user (hunter) menjalankan `hunter.c` sebelum `system.c` dijalankan. 

<br>
<img src = "https://github.com/user-attachments/assets/4eba56ab-48e7-460a-923b-f94fa35c3231" width = "400"> <br>
Program tidak berjalan karena system.c belum dijalankan. 
<br>

<img src = "https://github.com/user-attachments/assets/c7706d20-e46e-4f96-a920-5cfd0de1d3b7" width = "600"> <br>
Menu sederhana hunter.c (kiri) dan system.c (kanan) ketika awal dijalankan
<br>

![list dungeon](https://github.com/user-attachments/assets/e2d1b436-105e-4780-9132-9951b23a9f6b) <br>
Ini adalah stats default hunter di awal permainan.

```
void list_hunters(SystemData* data) {
    printf("\n== HUNTER INFO ==\n");
    for (int i = 0; i < data->num_hunters; ++i) {
        Hunter* h = &data->hunters[i];
        printf("Name: %-10s Level: %-2d EXP: %-3d ATK: %-3d HP: %-3d DEF: %-3d %s\n",
               h->username, h->level, h->exp, h->atk, h->hp, h->def,
               h->banned ? "[BANNED]" : "");
    }
}
```
Menampilkan semua hunter yang terdaftar dalam sistem. 
<br>
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
<br>
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

Fungsi ini menyetak dungeon yang telah di-generate di system, namun tidak semua dungeon yang telah digenerate oleh system.c dapat dilihat semua user, dungeon yang dapat dilihat bergantung dengan level user dan minimum level dungeonnya, semakin tinggi level user, semakin banyak pula dungeon yang bisa dilihat dan bisa digunakan. <br>

<img src = "https://github.com/user-attachments/assets/9c3df058-5937-4b21-a5f4-d79f26d3f28f" width = "600"> <br>
Gambar di atas meunjukkan bahwa terdapat 6 dungeon yang telah di-generate, namun kedua hunter (`oscar1` dan `oscar2`) hanya bisa melihat 1 dungeon saja sesuai dengan level mereka saat ini.

```
// hunter.c > raid_dungeon
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
```
Ini adalah fungsi yang akan terpanggil apabila hunter memilih opsi kedua (Dungeon Raid).
<br>
```
// hunter.c > raid_dungeon
 Dungeon* d = &data->dungeons[dungeon_idx];
    h->exp += d->exp;
    h->atk += d->atk;
    h->hp += d->hp;
    h->def += d->def;
    
    printf("\nRaid success! Gained:\n");
    printf("ATK: %d\nHP: %d\nDEF: %d\nEXP: %d\n", d->atk, d->hp, d->def, d->exp);
```
Hunter dipastikan menang melawan Dungeon dan mendapatkan stats secara random dengan constraint berikut:<br>

- ATK antara 150-300 <br>
- HP antara 100-150 <br>
- DEF antara 50-100 <br>
- EXP antara 25-50 <br>

![raid succes](https://github.com/user-attachments/assets/cd3bead1-055c-4c0e-a0ad-6b63e7f5e45d)

```
// void battle_hunter(SystemData* data, Hunter* attacker)
 printf("\n--- PVP LIST ---\n");
    for (int i = 0; i < data->num_hunters; ++i) {
        if (i != current_hunter_idx && !data->hunters[i].banned) {
            int power = data->hunters[i].atk + data->hunters[i].hp + data->hunters[i].def;
            printf("%s - Total Power: %d\n", data->hunters[i].username, power);
        }
    }
```
Potongan kode ini adalah salah satu dari bagian opsi ke-3 yaitu Hunter's Battle
![pvp list](https://github.com/user-attachments/assets/3d0726bd-5e08-45eb-b9f2-bfee3f0e3507) <br>

Hunter bisa memilih hunter lain untuk dikalahkan. Tips supaya menang adalah dengan memilih lawan dengan total power yang lebih rendah dari hunter yang dipakai saat ini, jika melawan hunter dengan total power yang lebih besar, maka hunter dengan power yang lebih kecil akan kalah dan keluar dari `hunter.c`. Hunter yang kalah akan dihapus dari sistem dan semua statsnya akan diberikan kepada hunter yang dilawannya.

 ### Gambaran jika hunter menang: 

![menang](https://github.com/user-attachments/assets/c3755344-0a57-4c30-b8de-3601a42e1c1b) <br>

Stats hunter yang menang akan ditambahkan

### Gambaran jika hunter kalah:

![kalah](https://github.com/user-attachments/assets/03ee33f9-aa88-4d4f-92d5-5877ce51bc30) <br>

Di game ini, system bisa banned atau unbanned hunter kapan saja <br>
<img src = "https://github.com/user-attachments/assets/222b335e-ef8a-4a5f-bbf3-5d2079e8374b" width = "500"> <br>
![baned2](https://github.com/user-attachments/assets/7ae1e33d-79cd-4157-8a1e-51846936a358) <br>
<img src = "https://github.com/user-attachments/assets/4b527381-77b5-4994-b916-b44417bd8434" width =  "500"> <br>

<img src = "https://github.com/user-attachments/assets/d86975b9-ffe5-477d-acc8-975cc553f47a" width = "400">

