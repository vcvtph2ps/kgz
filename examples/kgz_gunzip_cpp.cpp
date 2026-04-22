#include <fcntl.h>
#include <kgz_pub.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "Usage: %s <file.gz>\n", argv[0]);
        return 1;
    }

    const char* filepath = argv[1];
    int fd = open(filepath, O_RDONLY);
    if(fd == -1) {
        perror("open");
        return 1;
    }

    struct stat st;
    if(fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t filesize = (size_t) st.st_size;

    void* file = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if(file == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    uint64_t result_size; // size of decompressed data
    uint64_t buffer_size; // size of allocated buffer (may be larger than result_size)
    void* decompressed_data = kgz_gzip_decompress(file, filesize, &result_size, &buffer_size);

    if(decompressed_data == NULL) {
        printf("failed to decompress\n");
        munmap(file, filesize);
        close(fd);
        return 1;
    }

    char out_name[1024];
    snprintf(out_name, sizeof(out_name), "%s.ungzipped", filepath);
    int out_fd = open(out_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(out_fd == -1) {
        perror("open output");
        free(decompressed_data);
        munmap(file, filesize);
        close(fd);
        return 1;
    }

    ssize_t written = write(out_fd, decompressed_data, result_size);
    if(written == -1 || (uint64_t) written != result_size) {
        perror("write");
        close(out_fd);
        free(decompressed_data);
        munmap(file, filesize);
        close(fd);
        return 1;
    }

    close(out_fd);
    free(decompressed_data);
    munmap(file, filesize);
    close(fd);
    return 0;
}
