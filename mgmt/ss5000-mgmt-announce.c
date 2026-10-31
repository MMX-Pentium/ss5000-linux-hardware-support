#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

struct __attribute__((packed)) arp_frame {
	struct ethhdr ethernet;
	struct arphdr arp;
	uint8_t sender_mac[ETH_ALEN];
	uint8_t sender_ip[4];
	uint8_t target_mac[ETH_ALEN];
	uint8_t target_ip[4];
};

static int interface_data(int fd, const char *name, uint8_t mac[ETH_ALEN],
			  struct in_addr *address)
{
	struct ifreq ifr = { 0 };
	struct sockaddr_in *sin;

	if (strlen(name) >= IFNAMSIZ) {
		errno = ENAMETOOLONG;
		return -1;
	}

	strcpy(ifr.ifr_name, name);
	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
		return -1;
	if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
		errno = EAFNOSUPPORT;
		return -1;
	}
	memcpy(mac, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, name);
	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
		return -1;
	sin = (struct sockaddr_in *)&ifr.ifr_addr;
	if (sin->sin_family != AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}
	*address = sin->sin_addr;
	return 0;
}

static int send_announcement(int fd, int ifindex, const uint8_t mac[ETH_ALEN],
			     const struct in_addr *address, unsigned short opcode)
{
	static const uint8_t broadcast[ETH_ALEN] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};
	static const uint8_t empty_mac[ETH_ALEN] = { 0 };
	struct sockaddr_ll destination = {
		.sll_family = AF_PACKET,
		.sll_protocol = htons(ETH_P_ARP),
		.sll_ifindex = ifindex,
		.sll_halen = ETH_ALEN,
	};
	struct arp_frame frame = { 0 };
	const uint8_t *target_mac;
	ssize_t written;

	memcpy(destination.sll_addr, broadcast, ETH_ALEN);
	memcpy(frame.ethernet.h_dest, broadcast, ETH_ALEN);
	memcpy(frame.ethernet.h_source, mac, ETH_ALEN);
	frame.ethernet.h_proto = htons(ETH_P_ARP);

	frame.arp.ar_hrd = htons(ARPHRD_ETHER);
	frame.arp.ar_pro = htons(ETH_P_IP);
	frame.arp.ar_hln = ETH_ALEN;
	frame.arp.ar_pln = 4;
	frame.arp.ar_op = htons(opcode);

	target_mac = opcode == ARPOP_REPLY ? broadcast : empty_mac;
	memcpy(frame.sender_mac, mac, ETH_ALEN);
	memcpy(frame.sender_ip, address, sizeof(frame.sender_ip));
	memcpy(frame.target_mac, target_mac, ETH_ALEN);
	memcpy(frame.target_ip, address, sizeof(frame.target_ip));

	written = sendto(fd, &frame, sizeof(frame), 0,
			 (struct sockaddr *)&destination, sizeof(destination));
	if (written < 0)
		return -1;
	if ((size_t)written != sizeof(frame)) {
		errno = EIO;
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct in_addr address;
	uint8_t mac[ETH_ALEN];
	int fd, ifindex, round;

	if (argc != 2) {
		fprintf(stderr, "usage: %s INTERFACE\n", argv[0]);
		return EXIT_FAILURE;
	}

	ifindex = if_nametoindex(argv[1]);
	if (!ifindex) {
		fprintf(stderr, "%s: interface %s: %s\n", argv[0], argv[1],
			strerror(errno));
		return EXIT_FAILURE;
	}

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
	if (fd < 0) {
		fprintf(stderr, "%s: raw socket: %s\n", argv[0],
			strerror(errno));
		return EXIT_FAILURE;
	}

	if (interface_data(fd, argv[1], mac, &address) < 0) {
		fprintf(stderr, "%s: inspect %s: %s\n", argv[0], argv[1],
			strerror(errno));
		close(fd);
		return EXIT_FAILURE;
	}

	for (round = 0; round < 3; round++) {
		if (send_announcement(fd, ifindex, mac, &address,
				      ARPOP_REQUEST) < 0 ||
		    send_announcement(fd, ifindex, mac, &address,
				      ARPOP_REPLY) < 0) {
			fprintf(stderr, "%s: send on %s: %s\n", argv[0],
				argv[1], strerror(errno));
			close(fd);
			return EXIT_FAILURE;
		}
		if (round != 2)
			usleep(250000);
	}

	printf("announced %s on %s\n", inet_ntoa(address), argv[1]);
	close(fd);
	return EXIT_SUCCESS;
}
