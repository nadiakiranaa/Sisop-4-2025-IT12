    #define FUSE_USE_VERSION 35

    #include <fuse3/fuse.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <zlib.h>

    static const char *chiho_root = "/home/hosyi/chiho";
    #define SECRET_KEY "maimaipass"

    int is_starter_path(const char *path) { return strncmp(path, "/starter/", 9) == 0; }
    int is_metro_path(const char *path) { return strncmp(path, "/metro/", 7) == 0; }
    int is_dragon_path(const char *path) { return strncmp(path, "/dragon/", 8) == 0; }
    int is_heaven_path(const char *path) { return strncmp(path, "/heaven/", 8) == 0; }
    int is_blackrose_path(const char *path) { return strncmp(path, "/blackrose/", 11) == 0; }
    int is_youth_path(const char *path) { return strncmp(path, "/youth/", 7) == 0; }
    int is_7sref_path(const char *path) { return strncmp(path, "/7sref/", 7) == 0; }

    void redirect_7sref_path(const char *in_path, char *out_path) {
        const char *basename = in_path + 7;
        const char *underscore = strchr(basename, '_');
        if (!underscore) {
            strcpy(out_path, "/invalid_path");
            return;
        }
        char chiho[64];
        strncpy(chiho, basename, underscore - basename);
        chiho[underscore - basename] = '\0';
        snprintf(out_path, 1024, "/%s/%s", chiho, underscore + 1);
    }

    void shift_content_encode(char *buf, size_t size) { for (size_t i = 0; i < size; i++) buf[i] += (i % 256); }
    void shift_content_decode(char *buf, size_t size) { for (size_t i = 0; i < size; i++) buf[i] -= (i % 256); }
    void rot13(char *buf, size_t size) {
        for (size_t i = 0; i < size; i++) {
            if ('a' <= buf[i] && buf[i] <= 'z') buf[i] = 'a' + (buf[i] - 'a' + 13) % 26;
            else if ('A' <= buf[i] && buf[i] <= 'Z') buf[i] = 'A' + (buf[i] - 'A' + 13) % 26;
        }
    }

    int compress_data(const char *in, size_t in_size, char **out, size_t *out_size) {
        uLongf len = compressBound(in_size);
        *out = malloc(len);
        if (!*out) return -1;
        if (compress((Bytef *)*out, &len, (const Bytef *)in, in_size) != Z_OK) return -1;
        *out_size = len;
        return 0;
    }
    int decompress_data(const char *in, size_t in_size, char **out, size_t *out_size) {
        uLongf len = in_size * 4;
        *out = malloc(len);
        if (!*out) return -1;
        if (uncompress((Bytef *)*out, &len, (const Bytef *)in, in_size) != Z_OK) return -1;
        *out_size = len;
        return 0;
    }

    void build_real_path(const char *path, char *real_path) {
        if (is_starter_path(path)) snprintf(real_path, 1024, "%s%s.mai", chiho_root, path);
        else if (is_metro_path(path)) snprintf(real_path, 1024, "%s%s.ccc", chiho_root, path);
        else if (is_dragon_path(path)) snprintf(real_path, 1024, "%s%s.rot", chiho_root, path);
        else if (is_blackrose_path(path)) snprintf(real_path, 1024, "%s%s.bin", chiho_root, path);
        else if (is_heaven_path(path)) snprintf(real_path, 1024, "%s%s.enc", chiho_root, path);
        else if (is_youth_path(path)) snprintf(real_path, 1024, "%s%s.gz", chiho_root, path);
        else snprintf(real_path, 1024, "%s%s", chiho_root, path);
    }

    #define RESOLVE_PATH \
        const char *path_in = path; \
        char resolved_path[1024]; \
        if (is_7sref_path(path)) { redirect_7sref_path(path, resolved_path); path_in = resolved_path; }

    static int maimai_getattr(const char *path, struct stat *st, struct fuse_file_info *fi) {
        RESOLVE_PATH
        char real_path[1024];
        build_real_path(path_in, real_path);
        if (lstat(real_path, st) == -1) return -errno;
        return 0;
    }

    static int maimai_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                            off_t offset, struct fuse_file_info *fi,
                            enum fuse_readdir_flags flags) {
        RESOLVE_PATH
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
            } else if (is_metro_path(path_in) && strstr(de->d_name, ".ccc")) {
                char noext[256];
                strncpy(noext, de->d_name, strlen(de->d_name) - 4);
                noext[strlen(de->d_name) - 4] = '\0';
                filler(buf, noext, NULL, 0, 0);
            } else if (is_dragon_path(path_in) && strstr(de->d_name, ".rot")) {
                char noext[256];
                strncpy(noext, de->d_name, strlen(de->d_name) - 4);
                noext[strlen(de->d_name) - 4] = '\0';
                filler(buf, noext, NULL, 0, 0);
        } else if (is_blackrose_path(path_in) && strstr(de->d_name, ".bin")) {
                char noext[256];
                strncpy(noext, de->d_name, strlen(de->d_name) - 4);
                noext[strlen(de->d_name) - 4] = '\0';
                filler(buf, noext, NULL, 0, 0);
        } else if (is_heaven_path(path_in) && strstr(de->d_name, ".enc")) {
                char noext[256];
                strncpy(noext, de->d_name, strlen(de->d_name) - 4);
                noext[strlen(de->d_name) - 4] = '\0';
                filler(buf, noext, NULL, 0, 0);
        } else if (is_youth_path(path_in) && strstr(de->d_name, ".gz")) {
                char noext[256];
                strncpy(noext, de->d_name, strlen(de->d_name) - 4);
                noext[strlen(de->d_name) - 4] = '\0';
                filler(buf, noext, NULL, 0, 0);
            } else {
                filler(buf, de->d_name, NULL, 0, 0);
            }
        }
        closedir(dp);
        return 0;
    }

    static int maimai_open(const char *path, struct fuse_file_info *fi) {
        RESOLVE_PATH
        char real_path[1024];
        build_real_path(path_in, real_path);
        int fd = open(real_path, fi->flags);
        if (fd == -1) return -errno;
        close(fd);
        return 0;
    }

    static int maimai_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi) {
        RESOLVE_PATH
        char real_path[1024];
        build_real_path(path_in, real_path);

        if (is_heaven_path(path_in)) {
            char tmp[] = "/tmp/decryptedXXXXXX";
            int tmpfd = mkstemp(tmp);
            if (tmpfd == -1) return -errno;
            close(tmpfd);
            char cmd[2048];
            snprintf(cmd, sizeof(cmd),
                    "openssl enc -aes-256-cbc -d -salt -in '%s' -out '%s' -pass pass:%s 2>/dev/null",
                    real_path, tmp, SECRET_KEY);
            system(cmd);
            int fd = open(tmp, O_RDONLY);
            if (fd == -1) return -errno;
            int res = pread(fd, buf, size, offset);
            close(fd); unlink(tmp);
            return (res == -1) ? -errno : res;
        }
        if (is_youth_path(path_in)) {
            int fd = open(real_path, O_RDONLY);
            if (fd == -1) return -errno;
            char tmpbuf[65536];
            ssize_t bytes = read(fd, tmpbuf, sizeof(tmpbuf));
            close(fd);
            if (bytes <= 0) return -errno;
            char *dec; size_t dec_size;
            if (decompress_data(tmpbuf, bytes, &dec, &dec_size) != 0) return -EIO;
            memcpy(buf, dec + offset, size);
            free(dec);
            return (offset < dec_size) ? ((offset + size > dec_size) ? dec_size - offset : size) : 0;
        }
        int fd = open(real_path, O_RDONLY);
        if (fd == -1) return -errno;
        int res = pread(fd, buf, size, offset);
        if (res == -1) res = -errno;
        close(fd);
        if (is_metro_path(path_in)) shift_content_decode(buf, res);
        if (is_dragon_path(path_in)) rot13(buf, res);
        return res;
    }

    static int maimai_write(const char *path, const char *buf, size_t size, off_t offset,
                            struct fuse_file_info *fi) {
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
            snprintf(cmd, sizeof(cmd),
                    "openssl enc -aes-256-cbc -salt -in '%s' -out '%s' -pass pass:%s 2>/dev/null",
                    tmp, real_path, SECRET_KEY);
            system(cmd);
            unlink(tmp);
            return size;
        }
        if (is_youth_path(path_in)) {
            char *compressed; size_t compressed_size;
            if (compress_data(buf, size, &compressed, &compressed_size) != 0) return -EIO;
            int fd = open(real_path, O_WRONLY | O_CREAT, 0644);
            if (fd == -1) return -errno;
            int res = write(fd, compressed, compressed_size);
            free(compressed);
            close(fd);
            return res;
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

    static int maimai_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
        RESOLVE_PATH
        char real_path[1024];
        build_real_path(path_in, real_path);
        int fd = creat(real_path, mode);
        if (fd == -1) return -errno;
        close(fd);
        return 0;
    }

    static int maimai_unlink(const char *path) {
        RESOLVE_PATH
        char real_path[1024];
        build_real_path(path_in, real_path);
        int res = unlink(real_path);
        return (res == -1) ? -errno : 0;
    }

    static struct fuse_operations maimai_oper = {
        .getattr = maimai_getattr,
        .readdir = maimai_readdir,
        .open    = maimai_open,
        .read    = maimai_read,
        .write   = maimai_write,
        .create  = maimai_create,
        .unlink  = maimai_unlink,
    };

    int main(int argc, char *argv[]) {
        return fuse_main(argc, argv, &maimai_oper, NULL);
    }
    //
    //
    ///
    ///
    //
    ///
    //
    ///
    //
    //
    //
    //
    //
    //
    ////
    ///
    ///
    ///
    ///
    ///
    ////
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    ///
    /////
    /////
    //
    //
    //
    //
    ////
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    //
    ///
    //
    //
    ////

