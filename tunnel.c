#include <stdio.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h> //This contains a function to easily translate raw hex IPs into human-readable strings like "10.0.0.1"
#include <linux/ip.h> //This contains the iphdr struct blueprint
#include <sys/socket.h> // socket definition
#include <netinet/in.h> // network structures
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
    // Asks the OS kernel to allocate a network endpoint.
    // AF_INET = IPv4. SOCK_DGRAM = UDP (Connectionless).
    // Returns a raw file descriptor (soc) connected to the network stack.
    int soc=socket(AF_INET, SOCK_DGRAM, 0);
    if (soc<0) {
        printf("unexpected error opening socket\n");
    }
    // The 'mailing label' blueprint required by the OS to route our UDP packet.
    struct sockaddr_in server;
    // Wipe the memory block clean to prevent garbage data from corrupting our routing label.
    memset(&server, 0, sizeof(server));
    // Set Address Family to Internet (IPv4). Tells the router to expect a standard IP address.
    server.sin_family = AF_INET;
    // Host-TO-Network-Short. Flips the byte order of the port number from your
    // CPU's architecture (Little-Endian) to the standard Network architecture (Big-Endian).
    server.sin_port = htons(5555);
    // Crushes the human-readable string IP into a raw 32-bit integer for the network stack.
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    char buffer[2048]; //a memory buffer to catch the packet.
    while (1) {
        int byt= read(fd, buffer, sizeof(buffer)); //reading from your file descriptor (fd), into your buffer, up to the sizeof(buffer).
        if (byt<0) { //read() returns the exact number of bytes it caught. Store that in an integer variable.
            printf("read error\n");
        }if (byt>0) {
//buffer is just a dumb array of 2048 raw bytes. The compiler doesn't know it's a network packet.
            // Force the compiler to treat the raw byte array as an IP Header struct.
            // This allows us to read specific packet fields by name instead of raw byte offsets.
            struct iphdr *iph = (struct iphdr *)buffer; //iphdr is the Linux kernel's blueprint for an IP packet.
            // Check the very first field of the IP header. If it's not 4 (IPv4), ignore it.
            if (iph->version == 4) { //telling the compiler, Take this raw memory, lay the iphdr blueprint perfectly on top of it, and let me access the data using names instead of counting raw byte numbers."
                printf("IPv4\n");
                struct in_addr src, dest; //iph->saddr is the Source IP physically located inside the raw packet we just intercepted.
                // Copy the raw 32-bit IP addresses from the intercepted packet into our structs.
                // We use '->' because 'iph' is a pointer. We use '.' because 'src/dest' are direct structs.
                src.s_addr = iph->saddr; //src.s_addr is the designated holding slot inside the in_addr struct.
                dest.s_addr = iph->daddr;
                // inet_ntoa translates the raw 32-bit integer into a human-readable string (e.g., "10.0.0.1").
                // We print them sequentially to avoid C's static buffer overwrite trap.
                printf("Captured IPv4 Packet - Src: %s\n", inet_ntoa(src)); //Internet Network to ASCII
                printf("Captured IPv4 Packet - Dest: %s\n", inet_ntoa(dest));
                // The execution command.
                // Takes the exact number of intercepted bytes (byt) from our TUN memory (buffer),
                // attaches our target mailing label (&server), and blasts it out the physical network card.
                sendto(soc, buffer, byt, 0, (struct sockaddr *)&server, sizeof(server));
            }else {
                // If it's IPv6 or garbage, skip the rest of the loop and catch the next packet.
                continue;
            }

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