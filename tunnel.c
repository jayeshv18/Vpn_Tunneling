// Description: A bidirectional VPN daemon that intercepts raw IP packets from
// the Linux kernel, encrypts them via XOR, and tunnels them over UDP.
#include <stdio.h>       // Standard Input/Output. Provides printf() for terminal logging.
#include <linux/if.h>    // Linux Network Interfaces. Provides the 'ifreq' struct blueprint.
#include <linux/if_tun.h> // Linux TUN/TAP driver definitions. Provides the IFF_TUN macro.
#include <fcntl.h>       // File Control Operations. Provides O_RDWR (Open Read/Write) flags.
#include <string.h>      // String manipulation. Provides memset() and strcmp().
#include <sys/ioctl.h>   // Input/Output Control. Allows us to send direct hardware/kernel commands.
#include <unistd.h>      // POSIX standard API. Provides close(), read(), and write() functions.
#include <arpa/inet.h>   // Internet operations. Provides inet_ntoa() to convert hex IPs to strings.
#include <linux/ip.h>    // IP Header definitions. Provides the 'iphdr' struct blueprint.
#include <sys/socket.h>  // Core socket definitions. Provides socket(), bind(), sendto(), recvfrom().
#include <netinet/in.h>  // Network structures. Provides the 'sockaddr_in' blueprint.

// argc = Argument Count (how many words typed in terminal).
// argv = Argument Vector (array of the actual words typed, e.g., ["./tunnel", "-s"]).
int main(int argc, char *argv[]) {

    // Check if the user provided the required flag (-s or -c).
    if (argc < 2) {
        printf("Usage: %s [-s server | -c client]\n", argv[0]);
        return 1; // Exit with an error code if no flag is provided.
    }

    int soc;                  // Will hold the integer file descriptor for our UDP socket.
    char buffer[2048];        // A chunk of local memory (2KB) to physically hold the network packets.
    char key = 'X';           // The Pre-Shared Key (PSK) used for our bitwise XOR encryption.
    struct sockaddr_in server;// The "mailing label" struct used to route our UDP packets.
    int byt;                  // Will store the exact number of bytes intercepted or received.

    // open() asks the kernel for raw access to the TUN/TAP character device.
    // O_RDWR tells the OS we intend to both read packets from it and write packets to it.
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        printf("unexpected error opening tun\n");
        return fd;
    }

    // ifreq stands for InterFace REQuest. It is the struct used to configure network devices.
    struct ifreq ifr;

    // memset fills the memory block with zeroes. Always sanitize structs before using them!
    memset(&ifr, 0, sizeof(ifr));

    // Configure the flags for the virtual interface:
    // IFF_TUN: Commands the kernel to create a Layer 3 (IP Layer) device. Strips Ethernet headers.
    // IFF_NO_PI: Strips the default 4-byte kernel protocol info so byte [0] is the pure IP header.
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    // ioctl (Input/Output Control) sends the configuration directly to the kernel driver.
    // TUNSETIFF is a special macro that says "TUN Set Interface". This creates 'tun0'.
    int err = ioctl(fd, TUNSETIFF, (void *)&ifr);
    if (err < 0) {
        printf("unexpected error ioctl TUNSETIFF\n");
        close(fd);
        return err;
    }


    // socket() asks the OS to allocate a physical network connection.
    // AF_INET: We are using IPv4 routing.
    // SOCK_DGRAM: We want Datagrams (Connectionless UDP), not a stream (TCP).
    soc = socket(AF_INET, SOCK_DGRAM, 0);
    if (soc < 0) {
        printf("unexpected error opening socket\n");
    }


    // strcmp (String Compare) checks if the second terminal word (argv[1]) is "-c".
    if (strcmp(argv[1], "-c") == 0) {
        printf("[*] Starting VPN Client...\n");

        // **client setup**

        // Wipe the memory block clean to prevent garbage data from corrupting our routing label.
        memset(&server, 0, sizeof(server));

        // Address Family = Internet (IPv4).
        server.sin_family = AF_INET;

        // htons (Host TO Network Short). Flips the byte order of the port number
        // from your CPU's architecture (Little-Endian) to the Internet standard (Big-Endian).
        server.sin_port = htons(5555);

        // inet_addr crushes the human-readable string IP into a raw 32-bit integer.
        // This is the IP of our target VPN Server.
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        // Infinite loop: The client stays alive forever, waiting to intercept packets.
        while (1) {

            // read() pauses the program until the kernel routes a packet to our virtual interface.
            // When a packet arrives, it is pulled into our 'buffer'.
            byt = read(fd, buffer, sizeof(buffer));

            if (byt < 0) {
                printf("read error\n");
            }

            if (byt > 0) {

                // Force the compiler to treat the raw byte array as an IP Header struct.
                // This allows us to read specific packet fields by name (like ->saddr).
                struct iphdr *iph = (struct iphdr *)buffer;

                // Check the very first field of the IP header. If it is 4, it is IPv4.
                if (iph->version == 4) {
                    printf("IPv4\n");

                    // Create variables to hold the Source and Destination IPs.
                    struct in_addr src, dest;

                    // Copy the raw 32-bit IP addresses out of the packet into our variables.
                    src.s_addr = iph->saddr;
                    dest.s_addr = iph->daddr;

                    // inet_ntoa translates the raw 32-bit integer into a string (e.g., "10.0.0.1").
                    printf("Captured IPv4 Packet - Src: %s\n", inet_ntoa(src));
                    printf("Captured IPv4 Packet - Dest: %s\n", inet_ntoa(dest));

                    // Iterate through every single byte of the intercepted packet.
                    for (int z = 0; z < byt; z++) {
                        // Apply Bitwise XOR encryption.
                        // Overwrites the plaintext byte with the scrambled byte in memory.
                        buffer[z] = buffer[z] ^ key;
                    }

                    // sendto blasts the encrypted payload out of the physical network card
                    // toward the destination defined in our 'server' struct.
                    sendto(soc, buffer, byt, 0, (struct sockaddr *)&server, sizeof(server));
                } else {
                    // If it is IPv6 or random network noise, ignore it and wait for the next packet.
                    continue;
                }

                // hex dump
                // Prints the raw encrypted bytes to the terminal in hexadecimal format.
                for (int i = 0; i < byt; i++) {
                    // (unsigned char) prevents C from misinterpreting raw network bytes as negative numbers.
                    // %02x forces it to print exactly two hex characters per byte (e.g., "1d", "58").
                    printf("%02x", (unsigned char)buffer[i]);
                }
                printf("\n");
            }
        }
    }
// **server setup**
    else if (strcmp(argv[1], "-s") == 0) {
        printf("[*] Starting VPN Server...\n");

        // Always wipe the struct memory clean before assigning variables!
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(5555);
        // The server listens on 127.0.0.1 for local testing.
        server.sin_addr.s_addr = inet_addr("127.0.0.1");

        // bind() is critical for servers. It tells the OS kernel:
        // "I claim exclusive rights to UDP Port 5555. Route all arriving data to me."
        if (bind(soc, (struct sockaddr *)&server, sizeof(server)) < 0) {
            printf("unexpected error opening socket (Bind Failed)\n");
            return 1;
        }

        // recvfrom requires a special integer variable to store the size of the sender's address.
        socklen_t len = sizeof(server);

        // Infinite loop: The server stays alive forever, listening for encrypted payloads.
        while (1) {
            // recvfrom acts exactly like read. It pauses the program until data arrives
            // on port 5555, fills our buffer, and returns the exact number of bytes caught.
            int byt = recvfrom(soc, buffer, sizeof(buffer), 0, (struct sockaddr *)&server, &len);

            if (byt < 0) {
                printf("read error\n");
                return 1;
            }

            if (byt > 0) {

                // Iterate through every single byte of the received garbage data.
                for (int z = 0; z < byt; z++) {
                    // Because XOR is perfectly symmetric, XORing the scrambled data
                    // with the exact same key restores the original raw IP packet.
                    buffer[z] = buffer[z] ^ key;
                }


                // write() takes our freshly decrypted IP packet and shoves it directly
                // into the kernel's virtual interface (fd). The OS sees a valid packet
                // suddenly appear and routes it to its final destination.
                write(fd, buffer, byt);

                printf("Decrypted and injected %d bytes into kernel.\n", byt);
            }
        }
    }
    
    return 0;
}
