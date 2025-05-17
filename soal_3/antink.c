#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>

static const char *source_path = "/it24_host";
static const char *log_path = "/var/log/it24.log";

// Fungsi membalik string (in-place)
char *strrev(char *str) {
    int i = 0;
    int j = strlen(str) - 1;
    while (i < j) {
        char tmp = str[i];
        str[i++] = str[j];
        str[j--] = tmp;
    }
    return str;
}

// Fungsi log
void write_log(const char *message) {
    FILE *log = fopen(log_path, "a");
    if (log) {
        fprintf(log, "%s\n", message);
        fclose(log);
    }
}

// Mengecek apakah file berbahaya
int is_dangerous(const char *filename) {
    return strstr(filename, "nafis") || strstr(filename, "kimcun");
}

static int antink_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    char real_path[1024];
    snprintf(real_path, sizeof(real_path), "%s%s", source_path, path);
    return lstat(real_path, stbuf);
}

static int antink_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    char real_path[1024];
    snprintf(real_path, sizeof(real_path), "%s%s", source_path, path);

    DIR *dp = opendir(real_path);
    if (!dp) return -errno;

    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        char *display_name = strdup(de->d_name);

        if (is_dangerous(display_name)) {
            strrev(display_name);
            char logmsg[1024];
            snprintf(logmsg, sizeof(logmsg), "[WARNING] File %s terdeteksi sebagai berbahaya (dibalik)", de->d_name);
            write_log(logmsg);
        }

        filler(buf, display_name, NULL, 0, 0);
        free(display_name);
    }

    closedir(dp);
    return 0;
}

static int antink_open(const char *path, struct fuse_file_info *fi) {
    char real_path[1024];
    snprintf(real_path, sizeof(real_path), "%s%s", source_path, path);

    int fd = open(real_path, O_RDONLY);
    if (fd == -1) return -errno;

    fi->fh = fd;
    return 0;
}

static int antink_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    int fd = fi->fh;
    int res = pread(fd, buf, size, offset);
    if (res == -1) return -errno;

    if (!is_dangerous(path)) {
        for (int i = 0; i < res; i++) {
            if (isalpha(buf[i])) {
                char base = islower(buf[i]) ? 'a' : 'A';
                buf[i] = ((buf[i] - base + 13) % 26) + base;
            }
        }
    } else {
        char logmsg[1024];
        snprintf(logmsg, sizeof(logmsg), "[INFO] File %s dibaca tanpa enkripsi (berbahaya)", path);
        write_log(logmsg);
    }

    return res;
}

static int antink_release(const char *path, struct fuse_file_info *fi) {
    close(fi->fh);
    return 0;
}

static const struct fuse_operations antink_oper = {
    .getattr = antink_getattr,
    .readdir = antink_readdir,
    .open = antink_open,
    .read = antink_read,
    .release = antink_release,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &antink_oper, NULL);
}






















































































































































































































































































































































































//
