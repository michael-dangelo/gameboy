#include <stdio.h>

int main()
{
    unsigned char buffer[10];
    FILE *ptr = fopen("boot.bin", "rb");
    fread(buffer, sizeof(buffer), 1, ptr);
    for (int i = 0; i < 10; i++)
        printf("%x ", buffer[i]);
    printf("\n");
    fclose(ptr);
    return 0;
}
