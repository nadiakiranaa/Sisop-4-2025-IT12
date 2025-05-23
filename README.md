# Sisop-4-2025-IT12

### Soal Modul 4 Sistem Operasi 2025

## Anggota
| Nama                            | NRP        |
|---------------------------------|------------|
| Nadia Kirana Afifah Prahandita  | 5027241005 |
| Hansen Chang                    | 5027241028 |
| Muhammad Khosyi Syehab          | 5027241089 |

## PENJELASAN

## Soal_1
A. Buatlah virtual folder image/ dalam direktori mount yang berisi file gambar hasil konversi dari file .txt dalam folder anomali. Nama file virtual akan diberi format: <nama_file>_image_<timestamp>.png.
```cpp
if (strcmp(path, "/image") == 0) {
    ...
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt")) {
            ...
            sprintf(virtual_file_name, "%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png", base, ...);
            filler(buf, virtual_file_name, NULL, 0);
        }
    }
```
B. Saat file gambar virtual dibuka , konversi file .txt ke gambar .png dilakukan secara otomatis.
```cpp
if (strncmp(path, "/image/", 7) == 0) {
    ...
    char *image_path = generate_image_from_hex(txt_filename);
    ...
    int file_descriptor = open(image_path, O_RDONLY);
    ...
}
```
C. Tiap kali konversi file .txt ke .png berhasil, catat log ke file conversion.log yang berisi: timestamp, nama file .txt, dan nama file .png.
```cpp
FILE *log_fp = fopen(log_path, "a");
if (log_fp) {
    fprintf(log_fp, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted %s to %s\n", ...);
    fclose(log_fp);
}
```
D. Jika file .png yang dihasilkan dari konversi sudah ada, maka tidak perlu mengkonversi ulang. Gunakan file yang sudah ada.
```cpp
if (access(output_image_path, F_OK) == 0) {
    free(image_data);
    return output_image_path;
}
```
## HASIL OUTPUT ##

Struktur Akhir Directory 

![image](https://github.com/user-attachments/assets/311eea9c-b62d-47af-bae4-9d2aabdfebde)

Membaca Conversion.log
![image](https://github.com/user-attachments/assets/e43eac32-7ec2-4122-97af-5ccd54a216c6)
## Soal_2
I. Deskripsi Masalah dan Tujuan

Seorang ilmuwan menemukan pecahan data dari robot legendaris Baymax, tersimpan dalam direktori relics/ sebagai 14 potongan file berukuran 1KB dengan format Baymax.jpeg.000 hingga Baymax.jpeg.013. Tujuan sistem ini adalah membangkitkan file Baymax.jpeg secara utuh melalui mount point virtual (mount_dir) menggunakan FUSE, tanpa menyatukan file secara permanen.

Sistem harus dapat:
- Menampilkan file utuh Baymax.jpeg dari pecahan-pecahan
- Memecah file baru menjadi potongan 1KB saat ditulis
- Menghapus semua pecahan saat file dihapus
- Mencatat aktivitas (read, write, delete, copy) ke dalam activity.log



### Struktur Direktori yang Digunakan
├── mount_dir         # Mount point FUSE
├── relics            # Folder tempat menyimpan pecahan file
│   ├── Baymax.jpeg.000
│   ├── Baymax.jpeg.001
│   ├── ...
│   └── Baymax.jpeg.013
└── activity.log      # Catatan aktivitas pengguna

### Cara Kompilasi dan Menjalankan
Kompilasi:
```
gcc virtual_fs.c `pkg-config fuse --cflags --libs` -o virtual_fs
```
Menjalankan Filesystem:
```
./virtual_fs mount_dir
```

### 1. Menyatukan Pecahan Menjadi Satu File Virtual (Baymax.jpeg)
```
static int fs_getattr(const char* path, struct stat* st) {
    memset(st, 0, sizeof(struct stat));
    if (strcmp(path + 1, virtual_file) == 0) {
        st->st_mode = S_IFREG | 0444;
        st->st_nlink = 1;
        size_t total = 0;
        for (int i = 0; i < 14; i++) {
            char chunk_path[256];
            sprintf(chunk_path, "%s/%s.%03d", RELIC_DIR, virtual_file, i);
            FILE* f = fopen(chunk_path, "rb");
            if (f) {
                fseek(f, 0, SEEK_END);
                total += ftell(f);
                fclose(f);
            }
        }
        st->st_size = total;
        return 0;
    }
    return -ENOENT;
}
```
```
static int fs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    if (strcmp(path + 1, virtual_file) != 0)
        return -ENOENT;

    write_log("READ", virtual_file);
    size_t bytes_read = 0;
    size_t cur_offset = 0;
    for (int i = 0; i < 14; i++) {
        char chunk_path[256];
        sprintf(chunk_path, "%s/%s.%03d", RELIC_DIR, virtual_file, i);
        FILE* f = fopen(chunk_path, "rb");
        if (!f) continue;

        fseek(f, 0, SEEK_END);
        size_t len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (offset < cur_offset + len) {
            size_t skip = offset > cur_offset ? offset - cur_offset : 0;
            fseek(f, skip, SEEK_CUR);
            size_t to_read = len - skip;
            if (bytes_read + to_read > size)
                to_read = size - bytes_read;
            fread(buf + bytes_read, 1, to_read, f);
            bytes_read += to_read;
        }
        cur_offset += len;
        fclose(f);
        if (bytes_read >= size) break;
    }
    return bytes_read;
}
```
### 2. Memecah File Baru saat Ditulis
```
static int fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    char name[256];
    sscanf(path, "/%[^\n]", name);

    int part = 0;
    while (1) {
        char old_chunk[512];
        snprintf(old_chunk, sizeof(old_chunk), "%s/%s.%03d", SOURCE_DIR, name, part);
        if (access(old_chunk, F_OK) != 0)
            break;
        remove(old_chunk);
        part++;
    }

    part = 0;
    size_t written = 0;

    while (written < size) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", SOURCE_DIR, name, part);
        FILE *fp = fopen(chunk_path, "wb");
        if (!fp) break;

        size_t to_write = CHUNK_SIZE;
        if (size - written < CHUNK_SIZE)
            to_write = size - written;

        fwrite(buf + written, 1, to_write, fp);
        fclose(fp);

        written += to_write;
        part++;
    }

    char safe_name[MAX_NAME_LOG + 1];
    strncpy(safe_name, name, MAX_NAME_LOG);
    safe_name[MAX_NAME_LOG] = '\0';

    char chunks_written[512] = {0};
    if (part == 1) {
        snprintf(chunks_written, sizeof(chunks_written), "%s.000", safe_name);
    } else {
        snprintf(chunks_written, sizeof(chunks_written), "%s.000 to %s.%03d", safe_name, safe_name, part - 1);
    }

    write_log("WRITE: %s -> %s", safe_name, chunks_written);

    return size;
}
```
### 3. Menghapus Semua Pecahan File
```
static int fs_unlink(const char *path) {
    char name[256];
    sscanf(path, "/%[^\n]", name);

    int i = 0;
    while (1) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", SOURCE_DIR, name, i);
        if (access(chunk_path, F_OK) != 0) break;
        remove(chunk_path);
        i++;
    }

    if (i > 0) {
        write_log("DELETE: %s.000 - %s.%03d", name, name, i - 1);
    } else {
        write_log("DELETE: %s (no chunks found)", name);
    }

    return 0;
}
```
### 4. Mencatat Aktivitas Pengguna
```
static void write_log(const char *format, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);

    fprintf(log, "[%s] ", time_buf);

    va_list args;
    va_start(args, format);
    vfprintf(log, format, args);
    va_end(args);

    fprintf(log, "\n");
    fclose(log);
}
```
### 5. Menampilkan Baymax.jpeg 
```
static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    DIR *dp = opendir(SOURCE_DIR);
    if (!dp) return -errno;

    struct dirent *de;
    char listed[256][256];
    int listed_count = 0;

    while ((de = readdir(dp)) != NULL) {
        if (strstr(de->d_name, ".000")) {
            char base[256];
            strncpy(base, de->d_name, strlen(de->d_name) - 4);
            base[strlen(de->d_name) - 4] = '\0';

            int already_listed = 0;
            for (int i = 0; i < listed_count; ++i) {
                if (strcmp(listed[i], base) == 0) {
                    already_listed = 1;
                    break;
                }
            }
            if (!already_listed) {
                filler(buf, base, NULL, 0);
                strcpy(listed[listed_count++], base);
            }
        }
    }

    closedir(dp);
    return 0;
}
```
## Soal_3
## Soal_4
![image](https://github.com/user-attachments/assets/24d23dca-c8d9-49a9-a728-0d51ca7e7a19)
Pertama kita buat struktur direktori    
chiho/    
├── blackrose/       
├── dragon/    
├── heaven/       
├── metro/    
├── starter/    
└── youth/    
fuse_dir/    

Beberapa fungsi dibutuhkan untuk menjalankan soal ini, antara lain adalah `getattr`: Mendapatkan metadata file. `readdir`: Membaca isi direktori. `read`: Membaca isi file dari chiho. `write`: Menyimpan isi file ke chiho. `create`: Membuat file baru di area chiho sesuai aturan. `unlink`: Menghapus file (juga perlu menghapus transformasi jika ada).    
# build_real_path()
```
void build_real_path(const char *path, char *real_path) {
    if (is_starter_path(path)) snprintf(real_path, 1024, "%s%s.mai", chiho_root, path);
    else if (is_metro_path(path)) snprintf(real_path, 1024, "%s%s.ccc", chiho_root, path);
    else if (is_dragon_path(path)) snprintf(real_path, 1024, "%s%s.rot", chiho_root, path);
    else if (is_blackrose_path(path)) snprintf(real_path, 1024, "%s%s.bin", chiho_root, path);
    else if (is_heaven_path(path)) snprintf(real_path, 1024, "%s%s.enc", chiho_root, path);
    else if (is_youth_path(path)) snprintf(real_path, 1024, "%s%s.gz", chiho_root, path);
    else snprintf(real_path, 1024, "%s%s", chiho_root, path);
}
```
# define RESOLVE_PATH
Makro ini digunakan untuk Menangani redirect khusus pada area 7sref/. Menyederhanakan semua fungsi yang bekerja dengan path, supaya tidak perlu menulis kode redirect berulang-ulang. Menyediakan variabel path_in yang sudah siap digunakan, baik untuk path normal maupun path hasil redirect.
```
#define RESOLVE_PATH \
    const char *path_in = path; \
    char resolved_path[1024]; \
    if (is_7sref_path(path)) { \
        redirect_7sref_path(path, resolved_path); \
        path_in = resolved_path; \
    }
```
# maimai_readdir
Tugas utama dari funsi ini adlah Membaca isi direktori asli (chiho_root/...). Menyesuaikan nama file yang ditampilkan ke user sesuai aturan per chiho (misal: menghapus ekstensi .mai, .ccc, .rot, dst). Menyediakan nama file ke filler() untuk diteruskan ke user.
```
static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    RESOLVE_PATH

    if (is_7sref_path(path_in)) {
        struct {
            const char *chiho;
            const char *ext;
        } mapping[] = {
            { "starter", ".mai" },
            { "metro", ".ccc" },
            { "dragon", ".rot" },
            { "blackrose", ".bin" },
            { "heaven", ".enc" },
            { "youth", ".gz" },
        };
        for (int i = 0; i < 6; ++i) {
            char chiho_path[1024];
            snprintf(chiho_path, sizeof(chiho_path), "%s/%s", chiho_root, mapping[i].chiho);
            DIR *dp = opendir(chiho_path);
            if (!dp) continue;
            struct dirent *de;
            while ((de = readdir(dp)) != NULL) {
                if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
                if (strstr(de->d_name, mapping[i].ext)) {
                    char noext[256];
                    strncpy(noext, de->d_name, strlen(de->d_name) - strlen(mapping[i].ext));
                    noext[strlen(de->d_name) - strlen(mapping[i].ext)] = '\0';

                    char fake_name[512];
                    snprintf(fake_name, sizeof(fake_name), "%s_%s", mapping[i].chiho, noext);
                    filler(buf, fake_name, NULL, 0, 0);
                }
            }
            closedir(dp);
        }
        return 0;
    }

    char real_path[1024];
    snprintf(real_path, sizeof(real_path), "%s%s", chiho_root, path_in);
    DIR *dp = opendir(real_path);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;

        if (is_starter_path(path_in) && strstr(de->d_name, ".mai")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
            continue;
        } else if (is_metro_path(path_in) && strstr(de->d_name, ".ccc")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
	    continue;
        } else if (is_dragon_path(path_in) && strstr(de->d_name, ".rot")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
	    continue;
	} else if (is_blackrose_path(path_in) && strstr(de->d_name, ".bin")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
	    continue;
	} else if (is_heaven_path(path_in) && strstr(de->d_name, ".enc")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
	    continue;
	} else if (is_youth_path(path_in) && strstr(de->d_name, ".gz")) {
            char noext[256];
            strncpy(noext, de->d_name, strlen(de->d_name) - 4);
            noext[strlen(de->d_name) - 4] = '\0';
            filler(buf, noext, NULL, 0, 0);
	    continue;
        } else {
            filler(buf, de->d_name, NULL, 0, 0);
        }
    }
    closedir(dp);
    return 0;
}
```
# maimai_open
Mengecek apakah file benar-benar ada di chiho_root, sesuai dengan path yang diberikan (dan transformasi yang berlaku). Jika file tidak ada → kembalikan error. Jika ada → langsung tutup lagi (karena FUSE akan membukanya ulang secara internal).
```
static int maimai_open(const char *path, struct fuse_file_info *fi) {
    RESOLVE_PATH
    char real_path[1024];
    build_real_path(path_in, real_path);
    int fd = open(real_path, fi->flags);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}
```
# maimai_write
Tugas fungsi ini adalah Mengubah path menjadi path asli di disk (real_path). Memproses konten (buf) sesuai chiho:    
Heaven → AES-256 encrypt    
Youth → gzip compress    
Metro → custom shift encode    
Dragon → ROT13    
Lalu, Menyimpan hasilnya ke disk.
```
static int maimai_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    RESOLVE_PATH
    char real_path[1024];
    build_real_path(path_in, real_path);

    if (is_heaven_path(path_in)) {
        char tmp[] = "/tmp/plainXXXXXX";
        int tmpfd = mkstemp(tmp);
        if (tmpfd == -1) return -errno;
        write(tmpfd, buf, size);
        close(tmpfd);
        char cmd[2048];
        snprintf(cmd, sizeof(cmd), "openssl enc -aes-256-cbc -salt -in '%s' -out '%s' -pass pass:%s 2>/dev/null", tmp, real_path, SECRET_KEY);
        system(cmd);
        unlink(tmp);
        return size;
    }

    if (is_youth_path(path_in)) {
        if (is_youth_path(path_in)) {
    gzFile gz = gzopen(real_path, "wb");
    if (!gz) return -errno;
    int written = gzwrite(gz, buf, size);
    gzclose(gz);
    return (written == 0) ? -EIO : size;
}

    }

    char dir_path[1024];
    strcpy(dir_path, real_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir(dir_path, 0755);
    }

    int fd = open(real_path, O_WRONLY | O_CREAT, 0644);
    if (fd == -1) return -errno;
    char *temp = malloc(size);
    memcpy(temp, buf, size);
    if (is_metro_path(path_in)) shift_content_encode(temp, size);
    if (is_dragon_path(path_in)) rot13(temp, size);
    int res = pwrite(fd, temp, size, offset);
    if (res == -1) res = -errno;
    free(temp); close(fd);
    return res;
}
```
# maimai_create()
Membuat file baru di filesystem virtual maimai_fs, saat user menjalankan perintah seperti `touch /fuse_dir/starter/newfile.txt`
```
static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    RESOLVE_PATH
    char real_path[1024];
    build_real_path(path_in, real_path);

    char dir_path[1024];
    strcpy(dir_path, real_path);
    char *last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        mkdir_recursive(dir_path);
    }
    int fd = creat(real_path, mode);
    if (fd == -1) return -errno;
    close(fd);
    return 0;
}
```
# maimai_unlink()
Menghapus (unlink) file dari chiho yang sesuai. Dijalankan saat user menggunakan `rm /fuse_dir/heaven/secret.txt`
```
static int maimai_unlink(const char *path) {
    RESOLVE_PATH
    char real_path[1024];
    build_real_path(path_in, real_path);
    int res = unlink(real_path);
    return (res == -1) ? -errno : 0;
}
```
# Fuse Operations
```
static struct fuse_operations maimai_oper = {
    .getattr = maimai_getattr,
    .readdir = maimai_readdir,
    .open    = maimai_open,
    .read    = maimai_read, 
    .write   = maimai_write,
    .create  = maimai_create,
    .unlink  = maimai_unlink,
};
```
