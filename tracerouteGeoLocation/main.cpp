#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define PACKET_SIZE 64

#define ICMP_ECHO_REPLY 0
#define ICMP_DEST_UNREACH 3
#define ICMP_TTL_EXPIRE 11
#define ICMP_ECHO_REQUEST 8

typedef struct {
	unsigned char icmp_type;
	unsigned char icmp_code;
	unsigned short icmp_sum;
	unsigned short icmp_id;
	unsigned short icmp_seq;
	unsigned long timestamp; //Not part of ICMP
}icmphdr;

typedef struct {
	unsigned char		ip_hl : 4, ip_v : 4;
	unsigned char		ip_tos;
	unsigned short int  ip_len;
	unsigned short int  ip_id;
	unsigned short int  ip_off;
	unsigned char		ip_ttl;
	unsigned char		ip_p;
	unsigned short int  ip_sum;
	unsigned int		ip_src;
	unsigned int		ip_dst;
}iphdr;

uint16_t checksum(uint16_t* buffer, int size) {
	unsigned long cksum = 0;

	// Sum all the words together, adding the final byte if size is odd
	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}
	if (size) {
		cksum += *(UCHAR*)buffer;
	}

	// Do a little shuffling
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash
	return (USHORT)(~cksum);
}

int get_external_ip(in_addr* ipBuff) {
	char hn[80];
	if (gethostname(hn, sizeof(hn)) == SOCKET_ERROR) {
		printf("Could not get local hostname! Error: %ld\n", WSAGetLastError());
		return 1;
	}
	struct hostent *phe = gethostbyname(hn);
	if (phe == 0) {
		printf("Bad host lookup.\n");
		return 1;
	}

	if (sizeof(phe->h_addr_list) == 0) {
		printf("No connection!");
		return 1;
	}

	memcpy(ipBuff, phe->h_addr_list[0], sizeof(struct in_addr));
}

int run(int argc, char** argv) {
	char sendBuf[PACKET_SIZE]; memset(sendBuf, 0, sizeof(sendBuf));
	char recvBuf[1024]; memset(recvBuf, 0, sizeof(recvBuf));
	int ttl = 30;
	int seq_no = 0;
	if (argc > 2) {
		int temp = atoi(argv[2]);
		if (temp != 0)
			ttl = temp;
	}

	//Create raw socket.
	SOCKET rSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rSock == INVALID_SOCKET) {
		printf("Socket could not be created. Error: %ld\n", WSAGetLastError());
		return 1;
	}
	
	//Set ttl.
	if (setsockopt(rSock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR) {
		printf("Error setting socket options! Error: %ld\n", WSAGetLastError());
		return 1;
	}

	//Construct header
	icmphdr* sicmph = (icmphdr*)sendBuf;

	const char* msg = "Transgender pinging you!";

	memcpy(sendBuf + sizeof(icmphdr), msg, strlen(msg) + 1);
	sockaddr_in dest, from;
	in_addr srcAddr;

	if (get_external_ip(&srcAddr) == 1)
		return 1;

	dest.sin_family = AF_INET;
	dest.sin_port = htons(33434);
	inet_pton(AF_INET, argv[1], &dest.sin_addr);

	sicmph->icmp_type = ICMP_ECHO_REQUEST;
	sicmph->icmp_code = 0;
	sicmph->icmp_sum = 0;
	sicmph->icmp_id = (USHORT)GetCurrentProcessId();
	sicmph->icmp_seq = seq_no;
	sicmph->timestamp = GetTickCount();

	sicmph->icmp_sum = checksum((unsigned short*)sicmph, sizeof (sendBuf));

	printf("Sending packet to %s.\n", inet_ntoa(dest.sin_addr));
	int val = sendto(rSock, sendBuf, sizeof (sendBuf), 0, (sockaddr*)&dest, sizeof(dest));
	if (val == SOCKET_ERROR) {
		printf("Failed to send packet! Error: %ld\n", WSAGetLastError());
		return 1;
	}
	printf("%ld bytes sent.\n", val);

	int fromlen = sizeof(from);
	if (recvfrom(rSock, recvBuf, 1024, 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
		printf("Error receiving packets!");
		if (WSAGetLastError() == WSAEMSGSIZE) {
			printf("Buffer too small!\n");
		} else
			printf("Error: %ld", WSAGetLastError());
		return 1;
	}

	iphdr* riph = (iphdr*)recvBuf;
	unsigned short rhlen = riph->ip_hl*4;
	icmphdr* ricmph = (icmphdr*)(recvBuf + rhlen);

	if (ricmph->icmp_seq != seq_no)
		printf("Bad sequence number!\n");

	switch (ricmph->icmp_type) {
	case (ICMP_ECHO_REPLY):
		break;
	case (ICMP_DEST_UNREACH):
		printf("Destination unreachable!\n");
		return 1;
		break;
	case (ICMP_TTL_EXPIRE):
		break;
	default:
		printf("Unknown ICMP packet type!\n");
		return 1;
	}

	if (ricmph->icmp_id != (USHORT)GetCurrentProcessId());
		// Code to ignore packet from another pinger program.

	int nHops = int(256 - riph->ip_ttl);
	if (nHops == 192)
		// TTL 64, so ping was probably to a host on the LAN.
		// Call it a single hop.
		nHops = 1;

	else if (nHops == 128) {
		//Probably localHost
		nHops = 0;
	}

	printf("\n %d bytes from %s, icmp_seq, ", PACKET_SIZE, inet_ntoa(from.sin_addr), ricmph->icmp_seq);
	if (ricmph->icmp_type == ICMP_TTL_EXPIRE)
		printf("TTL Expired.\n");
	else {
		printf("%d hops", nHops);
		printf(", time: %ld ms", GetTickCount() - ricmph->timestamp);
	}

	return 0;
}

int __cdecl main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s <host> [ttl]\n", argv[0]);
		return 1;
	}

	WSADATA wsaData;
	int status = 0;

	if (WSAStartup(0x202, &wsaData) == 0)
	{
		status = run(argc, argv);
		WSACleanup();
	}
	else
	{
		printf("ERROR: WSA could not start!\n");
		status = 1;
	}
	return status;
}