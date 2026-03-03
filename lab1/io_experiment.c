#include <minix/syslib.h>

int main() {
    int ret = sys_outb(0x70, 0xA); // Write the value 0xA to I/O port 0x70
    printf("sys_outb returned: %d\n", ret);
    return 0;
}