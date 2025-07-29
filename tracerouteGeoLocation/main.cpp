#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

#define PACKET_SIZE 64

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

	//Create raw socket.
	SOCKET rSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rSock == INVALID_SOCKET) {
		printf("Socket could not be created. Error: %ld\n", WSAGetLastError());
		return 1;
	}
	
	BOOL bOptVal = TRUE;
	//Allow modification of IP header.
	//if (setsockopt(rSock, IPPROTO_IP, IP_HDRINCL, (char*)&bOptVal, sizeof(BOOL)) == SOCKET_ERROR) {
	if (setsockopt(rSock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == SOCKET_ERROR) {
		printf("Error setting socket options! Error: %ld\n", WSAGetLastError());
		return 1;
	}

	//Construct headers
	//iphdr* iph = (iphdr*)sendBuf;
	//icmphdr* icmph = (icmphdr*)(sendBuf + sizeof(iphdr));
	icmphdr* icmph = (icmphdr*)sendBuf;

	const char* msg = "Transgender pinging you!";

	//memcpy(sendBuf + sizeof (iphdr) + sizeof (icmphdr), msg, strlen(msg) + 1);
	memcpy(sendBuf + sizeof(icmphdr), msg, strlen(msg) + 1);
	sockaddr_in dest, source;
	in_addr srcAddr;

	if (get_external_ip(&srcAddr) == 1)
		return 1;

	dest.sin_family = AF_INET;
	dest.sin_port = htons(33434);
	inet_pton(AF_INET, argv[1], &dest.sin_addr);

	/*
	
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = sizeof (iphdr) + sizeof (icmphdr);
	iph->ip_id = 1;
	iph->ip_off = 0;
	iph->ip_ttl = 30;
	iph->ip_p = 1;
	iph->ip_sum = 0;
	inet_pton(AF_INET, inet_ntoa(srcAddr), &iph->ip_src);
	iph->ip_dst = dest.sin_addr.s_addr;
	*/

	icmph->icmp_type = 8; //ICMP_ECHO_REQUEST
	icmph->icmp_code = 0;
	icmph->icmp_sum = 0;
	icmph->icmp_id = (USHORT)GetCurrentProcessId();
	icmph->icmp_seq = 0;
	icmph->timestamp = GetTickCount();

	//iph->ip_sum = checksum((unsigned short*)&iph, sizeof (iphdr));
	icmph->icmp_sum = checksum((unsigned short*)icmph, sizeof (sendBuf));

	printf("Sending packet to %s.\n", inet_ntoa(dest.sin_addr));
	int val = sendto(rSock, sendBuf, sizeof (sendBuf), 0, (sockaddr*)&dest, sizeof(dest));
	if (val == SOCKET_ERROR) {
		printf("Failed to send packet! Error: %ld\n", WSAGetLastError());
		return 1;
	}
	printf("%ld bytes sent.\n", val);

	int fromlen = sizeof(source);
	if (recvfrom(rSock, recvBuf, 1024, 0, (sockaddr*)&source, &fromlen) == SOCKET_ERROR) {
		printf("Error receiving packets!");
		if (WSAGetLastError() == WSAEMSGSIZE) {
			printf("Buffer too small!\n");
		} else
			printf("Error: %ld", WSAGetLastError());
		return 1;
	}
	printf("Recieved response!\n");

	return 0;
}

int __cdecl main(int argc, char **argv) {
	if (argc != 2) {
		printf("Please provide an IPv4 address in dotted decimal format.\n");
		//return 1;
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