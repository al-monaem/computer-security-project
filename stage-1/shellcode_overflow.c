#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__attribute__((noinline))
void vulnerable(const char *filename) {
    unsigned char buffer[64];
    unsigned char input[256];
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    size_t n = fread(input, 1, sizeof(input), f);
    fclose(f);
    // Intentionally vulnerable: no bounds checking
    memcpy(buffer, input, n);
    printf("Copied %zu bytes into the buffer.\n", n);
    fflush(stdout);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <payload-file>\n", argv[0]);
        return 1;
    }
    vulnerable(argv[1]);
    puts("Normal execution completed.");
    return 0;
}
