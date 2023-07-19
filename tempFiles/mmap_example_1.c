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

    // 继续执行其他操作，而无需等待数据同步完成

    // 示例：读取文件内容
    char buffer[size];
    lseek(fd, 0, SEEK_SET); // 将文件指针重置到开头
    ssize_t bytesRead = read(fd, buffer, size);
    if (bytesRead == -1) {
        perror("read");
        munmap(addr, size);
        close(fd);
        return 1;
    }

    // 输出读取的文件内容
    printf("Read from file: %s\n", buffer);

    // 解除内存映射并关闭文件
    if (munmap(addr, size) == -1) {
        perror("munmap");
        close(fd);
        return 1;
    }
    
    close(fd);
    return 0;
}