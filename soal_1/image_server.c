#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>

#define PORT         8080
#define CHUNK_SIZE   4096
#define LOG_PATH     "server/server.log"
#define DB_DIR       "server/database/"

// tulis log ke file
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

// balik string
char* reverse_str(const char* s) {
    int n = strlen(s);
    char* r = malloc(n+1);
    for (int i = 0; i < n; i++) r[i] = s[n-1-i];
    r[n] = '\0';
    return r;
}

// decode hex -> binary
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

// tangani DECRYPT
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

// tangani DOWNLOAD
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

// baca seluruh request dari client
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

int main() {
    // siapkan direktori & log
    mkdir("server", 0755);
    mkdir("server/database", 0755);
    fprintf(stderr, "Starting server port %d...\n", PORT);

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) { perror("socket"); exit(1); }
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(sd, 10) < 0) {
        perror("listen"); exit(1);
    }
    fprintf(stderr, "Listening on port %d\n", PORT);

    while (1) {
        int client = accept(sd, NULL, NULL);
        if (client < 0) {
            perror("accept");
            continue;
        }
        if (fork() == 0) {
            close(sd);
            handle_client(client);
            exit(0);
        }
        close(client);
    }
    return 0;
}
