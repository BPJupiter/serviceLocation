#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

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
}icmphdr; /* 0x0010 */

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
}iphdr; /* 0x0014 */

typedef struct {
    unsigned int size;
    char src_addr[16];
    unsigned char seq_no;
    unsigned char hops;
    unsigned int rtt_ms;
    unsigned char icmp_type;
}replyInfo;

long get_tick_count() {
  long    ms;
  time_t  s;
  timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
  s = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6);
  if (ms > 999) {
      s++;
      ms = 0;
  }
  ms += s * 1000;
  return ms;
}

uint16_t checksum(uint16_t* buffer, int size) {
	unsigned long cksum = 0;

	// Sum all the words together, adding the final byte if size is odd
	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(unsigned short);
	}
	if (size) {
		cksum += *(unsigned char*)buffer;
	}

	// Do a little shuffling
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	// Return the bitwise complement of the resulting mishmash
	return (unsigned short)(~cksum);
}

int get_external_ip(char* ipBuff) {
	char hn[80];
	if (gethostname(hn, sizeof(hn)) == -1) {
		printf("Could not get local hostname! Error: %s\n", strerror(errno));
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

	memcpy(ipBuff, phe->h_addr_list[0], 32);
	return 0;
}

int construct_header(char* sendBuf, int sendBufSize, char* destAddr, int seq_no, sockaddr_in* dest) {
	icmphdr* sicmph = (icmphdr*)sendBuf;

	const char* msg = "Transgender pinging you!";

	memcpy(sendBuf + sizeof(icmphdr), msg, strlen(msg) + 1);

	dest->sin_family = AF_INET;
	dest->sin_port = htons(33434);
	inet_pton(AF_INET, destAddr, &(dest->sin_addr));

	sicmph->icmp_type = ICMP_ECHO_REQUEST;
	sicmph->icmp_code = 0;
	sicmph->icmp_sum = 0;
	sicmph->icmp_id = (unsigned short)getpid();
	sicmph->icmp_seq = seq_no;
  sicmph->timestamp = get_tick_count();

	sicmph->icmp_sum = checksum((unsigned short*)sicmph, sendBufSize);
	return 0;
}

int print_info(replyInfo output) {
  switch (output.icmp_type) {
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


  printf("\n%d bytes from %s, icmp_seq: %d, ", output.size, output.src_addr, output.seq_no);
  if (output.icmp_type == ICMP_TTL_EXPIRE)
    printf("TTL Expired, ");
  printf("%d hops, ", output.hops);
  // This method of latency calculation consistently overshoots wireshark's estimate.
  // I'm not a statistician!
  // NOTE: May also be WSL. Watch this space.
  printf("time: %d ms\n", output.rtt_ms);

  return 0;
}

int ping(int sock, sockaddr_in dest, int ttl, char* sendBuf, int sendBufSize, char* recvBuf, int recvBufSize, replyInfo* output) {
  sockaddr_in from;
  socklen_t fromlen = sizeof(from);
  long timeSent, timeRecv;

  if (setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&ttl, sizeof(ttl)) == -1) {
      printf("Error setting socket options! Error: %s\n", strerror(errno));
      return 1;
  }

  printf("Sending packet to %s.\n", inet_ntoa(dest.sin_addr));
	int val = sendto(sock, sendBuf, sendBufSize, 0, (sockaddr*)&dest, sizeof(dest));
  timeSent = get_tick_count();
	if (val == -1) {
		printf("Failed to send packet! Error: %s\n", strerror(errno));
		return 1;
	}
	printf("%d bytes sent.\n", val);

  val = recvfrom(sock, recvBuf, recvBufSize, 0, (sockaddr*)&from, &fromlen);
	if (val == -1) {
		printf("Error receiving packets!");
		if (errno == EMSGSIZE) {
			printf(" Buffer too small!\n");
		} else
			printf(" Error: %s\n", strerror(errno));
		return 1;
	}
  timeRecv = get_tick_count();
	printf("Recieved packet!\n");

  iphdr* riph = (iphdr*)recvBuf;
	unsigned short rhlen = riph->ip_hl*4;
	icmphdr* ricmph = (icmphdr*)(recvBuf + rhlen);

  int nHops = int(125 - riph->ip_ttl);
  if (ricmph->icmp_type == ICMP_TTL_EXPIRE)
      nHops = int(255 - riph->ip_ttl);

  output->size = val;
  strncpy(output->src_addr, inet_ntoa(from.sin_addr), 16);
  output->seq_no = ricmph->icmp_seq;
  output->hops = nHops;
  output->rtt_ms = timeRecv - timeSent;
  output->icmp_type = ricmph->icmp_type;

  return 0;
}

int run(int argc, char** argv) {
	char sendBuf[PACKET_SIZE]; memset(sendBuf, 0, sizeof(sendBuf));
	char recvBuf[1024]; memset(recvBuf, 0, sizeof(recvBuf));
	sockaddr_in dest, from, source;
	int ttl = 30;
	int seq_no = 0;
  long timeSent, timeRecv;

	if (argc > 2) {
		int temp = atoi(argv[2]);
		if (temp != 0)
			ttl = temp;
	}

	//Create raw socket.
	int sSock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sSock == -1) {
		printf("Socket could not be created. Error: %s\n", strerror(errno));
		return 1;
	}

    //Set socket timeout
	struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
	if (setsockopt(sSock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		printf("Could not set timeout value! Error: %s\n", strerror(errno));
		return 1;
	}

	construct_header(sendBuf, sizeof(sendBuf), argv[1], seq_no, &dest);
  replyInfo output;
  for (int i = 0; i < ttl; i++) {
    ping(sSock, dest, i, sendBuf, sizeof(sendBuf), recvBuf, sizeof(recvBuf), &output);
        
	  if (output.seq_no != seq_no)
		  printf("Bad sequence number!\n");

    if (print_info(output) == 1)
      return 1;

    if (strcmp(output.src_addr, argv[1]) == 0) {
      printf("Final hop!\n");
      break;
    }
  }
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		printf("Usage: %s <host> [ttl]\n", argv[0]);
		return 1;
	}

	int status = 0;
	status = run(argc, argv);

	return status;
}
