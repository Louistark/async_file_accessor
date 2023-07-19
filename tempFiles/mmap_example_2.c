#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int main() {
    // 打开文件
    int fd = open("example.txt", O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 将文件映射到内存
    size_t size = 4096; // 映射区域大小
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // 修改内存映射区域的数据
    sprintf(addr, "Hello, World!");

    // 将修改后的数据异步地同步到文件
    if (msync(addr, size, MS_ASYNC) == -1) {
        perror("msync");
        munmap(addr, size);
        close(fd);
        return 1;
    }

    // 调用 fsync 或 fdatasync 刷新文件描述符，等待数据写入磁盘
    if (fdatasync(fd) == -1) {
        perror("fdatasync");
        munmap(addr, size);
        close(fd);
        return 1;
    }

    // 解除内存映射并关闭文件
    if (munmap(addr, size) == -1) {
        perror("munmap");
        close(fd);
        return 1;
    }
    
    close(fd);
    return 0;
}