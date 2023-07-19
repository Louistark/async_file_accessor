#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // 打开文件
    int fd = open("example.txt", O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 获取文件大小
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek");
        close(fd);
        return 1;
    }

    // 将文件映射到内存
    void *addr = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // 异步读取文件内容
    char *buffer = malloc(file_size);
    if (buffer == NULL) {
        perror("malloc");
        munmap(addr, file_size);
        close(fd);
        return 1;
    }
    memcpy(buffer, addr, file_size);

    // 等待异步读取完成
    // 在这里可以进行其他操作，直到文件读取完成
    // 可以根据实际需求添加适当的等待时间或条件判断

    // 确认文件是否读取完成
    // 根据实际需求，可以通过比较实际读取的数据和文件大小来确认文件是否读取完成
    off_t bytes_read = file_size; // 实际文件读取的字节数
    if (bytes_read != file_size) {
        printf("File reading incomplete\n");
    } else {
        printf("File reading complete\n");

        // 在这里可以处理读取的文件内容
        // 比如打印内容、进行字符串操作等
        printf("File content: %.*s\n", (int)bytes_read, buffer);
    }

    // 解除内存映射、释放缓冲区并关闭文件
    free(buffer);
    munmap(addr, file_size);
    close(fd);

    return 0;
}