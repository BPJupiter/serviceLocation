#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <Ws2tcpip.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct {
	unsigned char icmp_type;
	unsigned char icmp_code;
	unsigned short icmp_sum;
	unsigned int icmp_rest;
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

uint16_t checksum(uint16_t* addr, int len) {
	int count = len;
	register uint32_t sum = 0;
	uint16_t answer = 0;

	while (count > 1) {
		sum += *(addr++);
		count -= 2;
	}

	if (count > 0) {
		sum += *(uint8_t*)addr;
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	answer = ~sum;

	return answer;
}

int run(int argc, char** argv) {
	char datagram[4096];
	char incoming[4096];

	//Create raw socket.
	SOCKET rSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (rSock == INVALID_SOCKET) {
		printf("Socket could not be created. Error: %ld\n", WSAGetLastError());
		return 1;
	}
	
	BOOL bOptVal = TRUE;
	//Allow modification of IP header.
	if (setsockopt(rSock, IPPROTO_IP, IP_HDRINCL, (char*)&bOptVal, sizeof(BOOL)) == SOCKET_ERROR) {
		printf("Error setting socket options! Error: %ld\n", WSAGetLastError());
		return 1;
	}

	/*
	//Get local ip address.
	sockaddr_in saServer;
	hostent* localHost;
	char* localIP;
	localHost = gethostbyname("");
	localIP = inet_ntoa(*(struct in_addr*)*localHost->h_addr_list);

	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = inet_addr(localIP);
	saServer.sin_port = htons(33434);

	if (bind(rSock, (SOCKADDR*)&saServer, sizeof (saServer)) == SOCKET_ERROR) {
		printf("Error binding local address to socket! Error: %ld\n", WSAGetLastError());
		return 1;
	}
	undiagnosed adhd function. fix later*/ 

	//Construct headers
	iphdr* iph = (iphdr*)datagram;
	icmphdr* icmph = (icmphdr*)(datagram + sizeof(iphdr));

	const char* msg = "Transgender pinging you!";

	memcpy(datagram + sizeof (iphdr) + sizeof (icmphdr), msg, strlen(msg) + 1);
	sockaddr_in sendTo;

	sendTo.sin_family = AF_INET;
	sendTo.sin_port = htons(33434);
	inet_pton(AF_INET, "127.0.0.1", &sendTo.sin_addr);
	
	iph->ip_hl = 5;
	iph->ip_v = 4;
	iph->ip_tos = 0;
	iph->ip_len = sizeof (iphdr) + sizeof (icmphdr);
	iph->ip_id = 1;
	iph->ip_off = 0;
	iph->ip_ttl = 255;
	iph->ip_p = 1;
	iph->ip_sum = 0;
	inet_pton(AF_INET, "127.0.0.1", &iph->ip_src);
	iph->ip_dst = sendTo.sin_addr.s_addr;

	icmph->icmp_type = 8;
	icmph->icmp_code = 0;
	icmph->icmp_sum = 0;
	icmph->icmp_rest = 0;

	iph->ip_sum = checksum((unsigned short*)&iph, sizeof (iphdr));
	icmph->icmp_sum = checksum((unsigned short*)&icmph, sizeof (icmph));

	int val = sendto(rSock, datagram, sizeof (datagram), 0, (sockaddr*)&sendTo, sizeof(sendTo));
	if (val == -1) {
		printf("Failed to send packet! Error: %ld\n", WSAGetLastError());
		return 1;
	}
	printf("%ld bytes sent.\n", val);
	


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