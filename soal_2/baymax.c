#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

#define SOURCE_DIR "/home/hansen/projek_sisop/relics"
#define LOG_FILE "/home/hansen/projek_sisop/activity.log"
#define CHUNK_SIZE 1024
#define MAX_CHUNKS 1000
#define MAX_NAME_LOG 200

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

static int fs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {

        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }

    char name[256];
    sscanf(path, "/%[^\n]", name);

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s.000", SOURCE_DIR, name);

    if (access(full_path, F_OK) == 0) {

        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;

        size_t size = 0;
        for (int i = 0; i < MAX_CHUNKS; ++i) {
            char chunk[512];
            snprintf(chunk, sizeof(chunk), "%s/%s.%03d", SOURCE_DIR, name, i);
            FILE *fp = fopen(chunk, "rb");
            if (!fp) break;
            fseek(fp, 0, SEEK_END);
            size += ftell(fp);
            fclose(fp);
        }
        stbuf->st_size = size;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        return 0;
    }

    return -ENOENT;
}

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

static int fs_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    char name[256];
    sscanf(path, "/%[^\n]", name);

    size_t total_read = 0;
    size_t current_offset = 0;

    for (int i = 0; i < MAX_CHUNKS; ++i) {
        char chunk_path[512];
        snprintf(chunk_path, sizeof(chunk_path), "%s/%s.%03d", SOURCE_DIR, name, i);
        FILE *fp = fopen(chunk_path, "rb");
        if (!fp) break;

        fseek(fp, 0, SEEK_END);
        size_t chunk_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        if (offset < current_offset + chunk_size) {
            size_t skip = offset > current_offset ? offset - current_offset : 0;
            fseek(fp, skip, SEEK_SET);
            size_t to_read = chunk_size - skip;
            if (to_read > size - total_read)
                to_read = size - total_read;
            fread(buf + total_read, 1, to_read, fp);
            total_read += to_read;
        }

        current_offset += chunk_size;
        fclose(fp);
        if (total_read >= size) break;
    }

    write_log("READ: %s", name);
    return total_read;
}

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

static int fs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char name[256];
    sscanf(path, "/%[^\n]", name);

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s.000", SOURCE_DIR, name);

    int fd = open(full_path, O_CREAT | O_WRONLY, mode);
    if (fd == -1)
        return -errno;
    close(fd);

    write_log("CREATE: %s", name);

    return 0;
}

static struct fuse_operations fs_oper = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .open = fs_open,
    .read = fs_read,
    .write = fs_write,
    .unlink = fs_unlink,
    .create = fs_create,
};

int main(int argc, char *argv[]) {
    umask(0);

    char *fuse_argv[argc + 2];
    int i;

    for (i = 0; i < argc; i++) {
        fuse_argv[i] = argv[i];
    }
    fuse_argv[i++] = "-o";
    fuse_argv[i++] = "allow_other";

    return fuse_main(i, fuse_argv, &fs_oper, NULL);
}
