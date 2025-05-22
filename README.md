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
## Soal_3
## Soal_4
