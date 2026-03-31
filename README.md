PROJECT TITLE: Custom Layer 3 Virtual Private Network Tunnel (User-Space Implementation)

PROJECT OVERVIEW
This project is a custom-built, user-space Virtual Private Network data plane written entirely in the C programming language. Rather than relying on high-level networking libraries, this software bridges the gap between the Linux kernel's networking stack and the physical network. It directly intercepts raw IP packets, parses their internal headers, and encapsulates them inside a custom UDP payload to be transmitted across a network. It serves as a foundational proof-of-concept for low-level systems programming, operating system kernel interactions, and manual network protocol manipulation.

SYSTEM ARCHITECTURE AND PIPELINE
The architecture of this tunnel is divided into four distinct operational phases:

Phase 1: The Kernel Trapdoor (Virtual Interface Allocation)
The program uses low-level system calls to command the Linux kernel to create a virtual network interface. The operating system treats this virtual interface exactly like a physical piece of hardware, such as a Wi-Fi or Ethernet card, allowing us to route standard network traffic directly into our custom software instead of out into the open internet.

Phase 2: The Interceptor (Packet Capture)
Once the virtual interface is established, the program enters a continuous listening state. It executes a blocking read loop to seamlessly pull raw, unadulterated network packets directly out of the kernel's memory buffer the exact moment before the operating system attempts to route them to a physical wire.

Phase 3: The Parser (Memory Overlay and Extraction)
The raw bytes captured from the kernel are meaningless on their own. The program casts this raw memory buffer into a standardized Linux IP header structure. This allows the software to programmatically extract vital routing information, such as the Source IP and Destination IP, without triggering memory scoping errors or relying on external packet-sniffing software.

Phase 4: The Transporter (UDP Encapsulation)
With the raw packet isolated, the program acts as a delivery mechanism. It initializes a standard user-space UDP socket, constructs a routing blueprint for a target server, and wraps the entire intercepted IP packet inside the payload section of a UDP datagram. It then transmits this encapsulated packet out through the machine's actual physical network card.

SYSTEM PREREQUISITES
To compile and execute this software, the host environment must have a Linux-based Operating System. It requires the GNU Compiler Collection to compile the source file. The user must also have root administrator privileges to authorize the creation of virtual network devices. Finally, standard networking tools like Netcat are recommended for testing the packet reception.

EXECUTION AND CONFIGURATION GUIDE
First, compile the source file using your standard C compiler to generate the executable program.

Second, run the newly created executable using root privileges. The terminal will pause and remain open as it establishes the virtual interface and begins listening for traffic.

Third, open a separate terminal window to configure the new virtual hardware. You must use standard Linux network administration commands to assign a local IP address to the virtual interface and power the link on. This instructs the operating system that the interface is active and ready to receive routed traffic.

TESTING THE ENCAPSULATION
To prove that the tunnel successfully intercepts and wraps network traffic, you can simulate a VPN server using Netcat.

Open a terminal and start a Netcat UDP listener on your designated testing port, instructing it to pipe the incoming data into a hexadecimal viewer.

Next, attempt to send a standard ping request to a dummy IP address that falls within the subnet you assigned to your virtual interface. The operating system will automatically route this ping request into your virtual interface, where your C program will instantly intercept it.

If successful, your C program terminal will output the cleanly parsed Source and Destination IP addresses of the ping request. Simultaneously, your Netcat listener terminal will instantly print a raw hexadecimal dump. This hex dump is the exact intercepted ping packet, successfully tunneled across a real UDP socket, verifying the Layer 3 encapsulation.

FUTURE ROADMAP
Phase 5: Cryptography Integration. The next architectural step is to implement a symmetric encryption cipher to mathematically scramble the raw IP packet buffer after interception but before UDP transmission. This ensures the payload is completely unreadable to any network sniffers monitoring the physical wire.

Phase 6: Bidirectional Routing. The final step involves building the server-side logic to receive the encrypted UDP packet, decrypt the payload back into its original raw state, and seamlessly inject it into the target server's own virtual interface to complete the tunnel.
