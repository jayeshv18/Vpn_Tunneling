PROJECT TITLE: Custom Layer 3 Virtual Private Network Tunnel (C)

OVERVIEW
Most networking happens through high-level abstractions. C-Sentry drops below those layers to the Network Layer (Layer 3). Instead of using standard libraries, this program manually hijacks the operating system's routing table, captures raw memory buffers, and applies custom bitwise cryptography to obfuscate data.

WHY THIS EXISTS:
To bypass censorship: Standard VPNs have "signatures" that firewalls can see. This custom XOR approach creates randomized noise that is much harder to detect.
To learn systems programming: Direct interaction with Kernel file descriptors and memory-mapped structures.

The architecture follows a 4-step pipeline:
• The Trapdoor (TUN Interface): The program uses the ioctl system call to create a virtual network interface (tun0). The Linux kernel treats this as a real network card, but instead of sending data to a wire, it sends it to our C program.

• The Parser: We cast raw memory onto the iphdr (IP Header) struct. This allows us to "see" the Source and Destination IP addresses inside the packet without any external tools.

• The Meat-Grinder (Encryption): We run every byte through a Bitwise XOR Cipher. This flips the bits of the packet based on a secret key, turning a readable web request into unreadable garbage.

• The Transporter (UDP): The scrambled packet is wrapped in a UDP datagram and sent to the remote Server, where the process is reversed and the packet is "injected" back into the internet.

USAGE INSTRUCTIONS
1. Requirements
• A Linux-based OS (Ubuntu/Debian recommended).
• Root/Sudo privileges (Required to create virtual interfaces).

2. Compilation
gcc -o vpn_tunnel tunnel.c

3. Running the Server (The Gateway)
• Run this on the machine that will act as your "exit point" (e.g., a Cloud VPS):
sudo ./vpn_tunnel -s
• Power on the interface
sudo ip addr add 10.0.0.1/24 dev tun0
sudo ip link set dev tun0 up

4. Running the Client (Your Device)
sudo ./vpn_tunnel -c
• Power on the interface
sudo ip addr add 10.0.0.2/24 dev tun1
sudo ip link set dev tun1 up

FUTURE IMPLEMENTATIONS:
• This project is an evolving architecture. Upcoming features include:
• Multi-threading (pthreads): Moving from a "one-way-at-a-time" pipe to a full-duplex system that can read and write simultaneously.
• I/O Multiplexing (select): Using event-driven programming to monitor multiple network sockets at once.
• AES-256-GCM: Upgrading from basic XOR to industry-standard encryption for military-grade security.
• Diffie-Hellman Handshake: Allowing the Client and Server to agree on a secret key automatically over the internet.
