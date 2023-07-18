#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int fd;
  struct stat sb;
  char *file_data;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file>\\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  if (fstat(fd, &sb) == -1) {
    perror("fstat");
    exit(EXIT_FAILURE);
  }

  file_data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (file_data == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  printf("%s", file_data);

  if (munmap(file_data, sb.st_size) == -1) {
    perror("munmap");
    exit(EXIT_FAILURE);
  }

  close(fd);
  exit(EXIT_SUCCESS);
}
