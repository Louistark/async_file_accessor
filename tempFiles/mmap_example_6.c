#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#define BUFSIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source_file> <dest_file>\\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int src_fd = open(argv[1], O_RDONLY);
    if (src_fd == -1) {
        perror("open source file");
        exit(EXIT_FAILURE);
    }

    int dst_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (dst_fd == -1) {
        perror("open dest file");
        exit(EXIT_FAILURE);
    }

    struct stat sb;
    if (fstat(src_fd, &sb) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    off_t offset = 0;
    size_t length = sb.st_size;
    void *src_addr = mmap(NULL, length, PROT_READ, MAP_PRIVATE, src_fd, offset);
    if (src_addr == MAP_FAILED) {
        perror("mmap source file");
        exit(EXIT_FAILURE);
    }

    void *dst_addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, dst_fd, offset);
    if (dst_addr == MAP_FAILED) {
        perror("mmap dest file");
        exit(EXIT_FAILURE);
    }

    // 将文件描述符设置为非阻塞模式
    int flags = fcntl(dst_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(dst_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }

    // 异步写入文件
    size_t remain = length;
    char *src = (char *) src_addr;
    char *dst = (char *) dst_addr;
    while (remain > 0) {
        ssize_t n = write(dst_fd, src, BUFSIZE);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            } else if (errno == EAGAIN) {
                fd_set set;
                FD_ZERO(&set);
                FD_SET(dst_fd, &set);
                if (select(dst_fd + 1, NULL, &set, NULL, NULL) == -1) {
                    perror("select");
                    exit(EXIT_FAILURE);
                }
                continue;
            } else {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        remain -= n;
        src += n;
        dst += n;
    }

    // 将数据刷新到磁盘
    if (fsync(dst_fd) == -1) {
        perror("fsync");
        exit(EXIT_FAILURE);
    }

    // 释放内存映射区域
    if (munmap(src_addr, length) == -1) {
        perror("munmap source file");
        exit(EXIT_FAILURE);
    }
    if (munmap(dst_addr, length) == -1) {
        perror("munmap dest file");
        exit(EXIT_FAILURE);
    }

    close(src_fd);
    close(dst_fd);

    return 0;
}
