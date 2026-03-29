#include <stdio.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
int main() {
    int fd = open("/dev/net/tun", O_RDWR); //Open for Reading and Writing.
    if (fd < 0) {
        printf("unexpected error opening tun\n");
        return fd;
    }//ifreq stands for InterFace REQuest.
    //ifr is just the variable name

    /*Look up the ifreq blueprint from the Linux headers,
     calculate exactly how many bytes of RAM it requires,
     carve out that chunk of memory on the stack right now,
     and label it ifr.*/

    struct ifreq ifr; // declare
    memset(&ifr, 0, sizeof(ifr)); // sanitize and store all 0's about the about the size of it.
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // set and configure flags

    /* IFF_TUN commanding the Linux kernel to create a Layer 3 (Network Layer) device.
     This means when the kernel hands data to your C program,
     it strips away all the Ethernet MAC addresses and hands you a pure,
     raw IP packet.*/

    /*f you passed IFF_TAP, the kernel would create a Layer 2 (Data Link) device,
     forcing you to manually parse and construct Ethernet frames before you even get to the IP data.
     We use IFF_TUN because our goal is to route IP packets, not build a virtual switch.*/

    /* IFF_NO_PI By default, the kernel prepends 4 bytes of metadata (protocol info) to every packet. Passing IFF_NO_PI strips those 4 bytes.
     This ensures that byte 0 in your memory buffer is the absolute start of the raw IP header,
     giving you the pure packet exactly as it appears on the physical wire.*/

    int err= ioctl(fd, TUNSETIFF, (void *)&ifr); //TUNSETIFF stands for TUN Set Interface. It is a unique integer macro understood only by the kernel's TUN/TAP driver.
    if (err<0) {
        printf("unexpected error ioctl TUNSETIFF\n");
        close(fd);
        return err;
    }
    char buffer[2048]; //a memory buffer to catch the packet.
    while (1) {
        int byt= read(fd, buffer, sizeof(buffer)); //reading from your file descriptor (fd), into your buffer, up to the sizeof(buffer).
        if (byt<0) { //read() returns the exact number of bytes it caught. Store that in an integer variable.
            printf("read error\n");
        }if (byt>0) {

            /*If your read() function caught data (bytes > 0),
             iterate through that buffer using a for loop.
             Print every single byte in hexadecimal using printf("%02X ", (unsigned char)buffer[i]);*/

            for (int i=0; i<byt; i++) {
                printf("%02x", (unsigned char)buffer[i]); /*the char type is typically signed by default.
                It holds values from -128 to 127. Network packets are raw binary. C program reads a byte like 11000000 (0xC0),
                it sees the first bit is a 1 and thinks, this is a negative number! When we pass it to printf using %02x.
                It pads the negative number with Fs to make it fit into a 32-bit integer, printing FFFFFFC0 instead of C0.*/

            }
            printf("\n");
        }
    }
    return 0;
}