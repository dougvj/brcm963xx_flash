#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>

#define DISABLE_WRITE

typedef enum {
  BCM_IMAGE_WHOLE=5, // The only one activated, use for writing only
  FLASH_SIZE=7, // Get the size of the flash
} board_action;

#define FLASH_BASE 0xB8000000


typedef struct {
  char* buf; // originally 'string'
  void* data; // originally 'buf', apparently used to put ptrs
  int len; // originally strLen
  int offset;
  board_action action; //called 'action' in bcrm source
  int result;
} board_ioctl_params;

#define BOARD_IOCTL_FLASH_WRITE _IOWR('B', 0, board_ioctl_params)
#define BOARD_IOCTL_FLASH_READ _IOWR('B', 1, board_ioctl_params)

void print_usage_and_quit(const char* exec_name) {
  fprintf(stderr, "Usage: %s <-r/-w> <image-file>\n"
                  "\tUse -r to read flash, -w to write flash\n"
                  "\tIf you want to dump to stdout, use -stdout as the image "
                  "file name\n",
/*                  "\n\n"
                  "Disclaimer: This utility does not verify any flash image, "
                  "use it to write\n(especially WHOLE or CFE) with extreme "
                  "care\n",*/
                  exec_name);
  exit(1);
}


int read_flash_size(int board_fd) {
  int len = 0;
  board_ioctl_params board_params = {
    .action = FLASH_SIZE,
    .data = &len,
  };
  int ret;
  if ((ret = ioctl(board_fd, BOARD_IOCTL_FLASH_READ, &board_params)) != 0) {
    fprintf(stderr, "ioctl failed with return %d (1)\n", ret);
    exit(1);
  }
  if (board_params.result != 0) {
    fprintf(stderr, "could not read flash size: %d\n", board_params.result);
    exit(1);
  }
  return len;
}

void read_flash(char* buf, int len) {
  int mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
  if (mem_fd == -1) {
    fprintf(stderr, "unable to open /dev/mem\n");
    exit(1);
  }
  char* src = mmap(NULL, len, PROT_READ, MAP_SHARED, mem_fd, FLASH_BASE);
  if (!src) {
    fprintf(stderr, "unable to map flash\n");
    exit(1);
  }
  memcpy(buf, src, len);
  munmap(src, len);
  close(mem_fd);
}

void write_flash(int board_fd, char* buf, int len) {
  board_ioctl_params board_params = {
    .buf = buf,
    .len = len,
    .action = BCM_IMAGE_WHOLE,
  };
  int ret;
  if ((ret = ioctl(board_fd, BOARD_IOCTL_FLASH_WRITE, &board_params)) != 0) {
    fprintf(stderr, "ioctl failed with return %d (2)\n", ret);
    exit(1);
  }
  if (board_params.result != board_params.len) {
    fprintf(stderr, "ioctl did not read entire flash, only %d of %d bytes\n",
                    board_params.result,
                    board_params.len);
    exit(1);
  }
}

int main(int argc, char** argv) {
  if (argc != 3) {
    print_usage_and_quit(argv[0]);
  }
  const char* mode = argv[1];
  const char* filename = argv[2];
  int board_fd = open("/dev/brcmboard", O_RDWR);
  if (board_fd == -1) {
    fprintf(stderr, "could not find bcrm device\n");
    exit(1);
  }
  int len = read_flash_size(board_fd);
  fprintf(stderr, "Flash size is: %d\n", len);
  char* image_data = malloc(len);
  if (!image_data) {
    fprintf(stderr, "could not alloc memory\n");
    exit(1);
  }
  if (strcmp(mode, "-r") == 0) {
    // read mode
    int out_fd;
    if (strcmp(filename, "-stdout") == 0) {
      out_fd = fileno(stdout);
    } else {
      out_fd = open(filename, O_WRONLY | O_CREAT);
    }
    if (out_fd == -1) {
      fprintf(stderr, "could not open %s\n", filename);
      exit(1);
    }
    read_flash(image_data, len);
    write(out_fd, image_data, len);
    close(out_fd);
  }
  if (strcmp(mode, "-w") == 0) {
    // write mode
    int in_fd = open(filename, O_RDONLY);
    if (in_fd == -1) {
      fprintf(stderr, "could not open '%s'\n", filename);
    }
    int size = lseek(in_fd, 0, SEEK_END);
    if (size != len) {
      fprintf(stderr, "file size mismatch. Expected %d got %d\n", len, size);
      exit(1);
    }
#ifdef DISABLE_WRITE
    fprintf(stderr, "write mode is disabled\n");
    exit(1);
#endif
    if (read(in_fd, image_data, len) != len) {
      fprintf(stderr, "could not read file\n");
      exit(1);
    }
    write_flash(board_fd, image_data, len);
    close(in_fd);
  } else {
    fprintf(stderr, "unknown mode: %s\n", mode);
    exit(1);
  }
  close(board_fd);
}
