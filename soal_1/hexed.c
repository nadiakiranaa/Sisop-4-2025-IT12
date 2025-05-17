#define FUSE_USE_VERSION 28
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static const char *root_dir = "/home/nadia/Sisop-4-2025-IT12/soal_1/anomali";
static const char *log_file = "conversion.log";

unsigned char convert_hex_char(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

unsigned char *convert_hex_to_bytes(const char *hex, size_t *output_length) {
    *output_length = strlen(hex) / 2;
    unsigned char *result = malloc(*output_length);
    for (size_t i = 0; i < *output_length; i++) {
        result[i] = (convert_hex_char(hex[2 * i]) << 4) | convert_hex_char(hex[2 * i + 1]);
    }
    return result;
}

char *generate_image_from_hex(const char *text_filename) {
    static char output_image_path[1024];
    char text_file_path[1024];
    char image_folder[1024];
    sprintf(image_folder, "%s/image", root_dir);
    mkdir(image_folder, 0755);

    sprintf(text_file_path, "%s/%s", root_dir, text_filename);

    FILE *hex_file = fopen(text_file_path, "r");
    if (!hex_file) return NULL;

    char hex_buffer[8000];
    size_t read_len = fread(hex_buffer, 1, sizeof(hex_buffer) - 1, hex_file);
    hex_buffer[read_len] = '\0';
    fclose(hex_file);

    size_t image_data_length;
    unsigned char *image_data = convert_hex_to_bytes(hex_buffer, &image_data_length);

    char basename[512];
    strcpy(basename, text_filename);
    char *dot_position = strrchr(basename, '.');
    if (dot_position) *dot_position = '\0';

    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);

    sprintf(output_image_path, "%s/%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png",image_folder, basename,time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,            time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

    if (access(output_image_path, F_OK) == 0) {
        free(image_data);
        return output_image_path;
    }

    FILE *image_file = fopen(output_image_path, "wb");
    if (!image_file) {
        free(image_data);
        return NULL;
    }
    fwrite(image_data, 1, image_data_length, image_file);
    fclose(image_file);
    free(image_data);

    // Logging
    char log_path[1024];
    sprintf(log_path, "%s/%s", root_dir, log_file);

    FILE *log_fp = fopen(log_path, "a");
    if (log_fp) {
        fprintf(log_fp, "[%04d-%02d-%02d][%02d:%02d:%02d]: Successfully converted %s to %s\n",time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday,time_info->tm_hour, time_info->tm_min, time_info->tm_sec,text_filename, strrchr(output_image_path, '/') + 1);
        fclose(log_fp);
    }

    return output_image_path;
}

static int fs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0 || strcmp(path, "/.") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strcmp(path, "/image") == 0 || strcmp(path, "/image/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (strncmp(path, "/image/", 7) == 0) {
        const char *filename = path + 7;
        char filename_buffer[512];
        strncpy(filename_buffer, filename, sizeof(filename_buffer) - 1);
        filename_buffer[sizeof(filename_buffer) - 1] = '\0';

        char *timestamp_pos = strstr(filename_buffer, "image");
        if (!timestamp_pos) return -ENOENT;
        *timestamp_pos = '\0';

        char original_txt_name[512];
        sprintf(original_txt_name, "%s.txt", filename_buffer);

        char original_txt_path[1024];
        sprintf(original_txt_path, "%s/%s", root_dir, original_txt_name);

        if (access(original_txt_path, F_OK) == 0) {
            char *image_path = generate_image_from_hex(original_txt_name);
            if (!image_path) return -ENOENT;

            int result = lstat(image_path, stbuf);
            if (result == -1) return -errno;
            return 0;
        } else {
            return -ENOENT;
        }
    }

    char absolute_path[1024];
    sprintf(absolute_path, "%s%s", root_dir, path);
    int result = lstat(absolute_path, stbuf);
    if (result == -1) return -errno;
    return 0;
}

static int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "image", NULL, 0);

        DIR *dir = opendir(root_dir);
        if (!dir) return -errno;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, "image") == 0) continue;
            filler(buf, entry->d_name, NULL, 0);
        }
        closedir(dir);
        return 0;
    }

    if (strcmp(path, "/image") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        DIR *dir = opendir(root_dir);
        if (!dir) return -errno;

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".txt")) {
                char base[512];
                strcpy(base, entry->d_name);
                char *ext = strrchr(base, '.');
                if (ext) *ext = '\0';

                time_t current_time = time(NULL);
                struct tm *time_info = localtime(&current_time);

                char virtual_file_name[1024];
                sprintf(virtual_file_name, "%s_image_%04d-%02d-%02d_%02d:%02d:%02d.png", base, time_info->tm_year + 1900, time_info->tm_mon + 1, time_info->tm_mday, time_info->tm_hour, time_info->tm_min, time_info->tm_sec);

                filler(buf, virtual_file_name, NULL, 0);
            }
        }
        closedir(dir);
        return 0;
    }

    char absolute_path[1024];
    sprintf(absolute_path, "%s%s", root_dir, path);
    DIR *dir = opendir(absolute_path);
    if (!dir) return -errno;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        filler(buf, entry->d_name, NULL, 0);
    }
    closedir(dir);
    return 0;
}

static int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    (void) fi;

    if (strncmp(path, "/image/", 7) == 0) {
        const char *filename = path + 7;

        char filename_copy[512];
        strncpy(filename_copy, filename, sizeof(filename_copy) - 1);
        filename_copy[sizeof(filename_copy) - 1] = '\0';

        char *pos = strstr(filename_copy, "image");
        if (!pos) return -ENOENT;
        *pos = '\0';

        char txt_filename[512];
        sprintf(txt_filename, "%s.txt", filename_copy);

        char full_txt_path[1024];
        sprintf(full_txt_path, "%s/%s", root_dir, txt_filename);

        if (access(full_txt_path, F_OK) == 0) {
            char *image_path = generate_image_from_hex(txt_filename);
            if (!image_path) return -ENOENT;

            int file_descriptor = open(image_path, O_RDONLY);
            if (file_descriptor == -1) return -errno;

            int result = pread(file_descriptor, buf, size, offset);
            if (result == -1) result = -errno;

            close(file_descriptor);
            return result;
        } else {
            return -ENOENT;
        }
    }

    char absolute_path[1024];
    sprintf(absolute_path, "%s%s", root_dir, path);

    int fd = open(absolute_path, O_RDONLY);
    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    close(fd);
    return res;
}

static struct fuse_operations fs_operations = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read = fs_read,
};

int main(int argc, char *argv[]) {
    umask(0);
    return fuse_main(argc, argv, &fs_operations, NULL);
}
