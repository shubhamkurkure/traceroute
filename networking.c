#include "networking.h"
static uint16_t inet_cksum(
	const uint16_t *addr,
	const size_t len,
	const uint16_t csum
)
{
	register size_t nleft = len; 
	const uint16_t *w = addr;
	register uint16_t answer;
	register int sum = csum;

	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		sum += htons(*(uint8_t *)w << 8);
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;
	return answer;
}

void sendtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl, const uint seq
)
{

	struct icmp packet;
	packet.icmp_type = ICMP_ECHO;
	packet.icmp_code = 0;
	packet.icmp_id = getpid();
	packet.icmp_seq = (ttl << SEQ_LEN) | seq;
	packet.icmp_cksum = 0;
	packet.icmp_cksum = inet_cksum((ushort *)&packet, 8, 0);


	handle(
		"set ttl",
		setsockopt(rawsock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl))
	);
	handle(
		"send packet",
		sendto(rawsock, &packet, 8, 0, &addr, sizeof(addr))
	);
}


void sendmtr(
	const int rawsock,
	const struct sockaddr_in addr,
	const uint8_t ttl,
	const uint reqc
)
{
	for (uint i = 0; i < reqc; i++) {
		sendtr(rawsock, addr, ttl, i);
	}
}


static uint8_t rbuffer[IP_MAXPACKET + 1];

int recvtr(
	const int rawsock,
	const uint8_t ttl,
	struct in_addr *addr
)
{
	int retv = recv(rawsock, rbuffer, IP_MAXPACKET, MSG_DONTWAIT);
	if (retv < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		return PACK_NONE;
	} else {
		handle("recive packet", retv);
	}

	struct ip *ipp = (struct ip *)rbuffer;
	struct icmp *icmpp = (struct icmp *)((uint8_t *)ipp + IP_HLEN(*ipp));

	if (
		icmpp->icmp_type == ICMP_TIME_EXCEEDED
		&&
		icmpp->icmp_code == ICMP_EXC_TTL
	) {

		struct ip *ipo = (struct ip *)((uint8_t *)icmpp + ICMP_HLEN);
		struct icmp *icmpo = (struct icmp *)((uint8_t *)ipo + IP_HLEN(*ipo));

		if (icmpo->icmp_id == getpid() && (icmpo->icmp_seq >> SEQ_LEN) == ttl) {
			*addr = ipp->ip_src;
			return PACK_NONDEST;
		}
	} else if (
		icmpp->icmp_id == getpid()
		&&
		(icmpp->icmp_seq >> SEQ_LEN) == ttl
		&&
		icmpp->icmp_type == ICMP_ECHOREPLY
	) {
		*addr = ipp->ip_src;
		return PACK_DEST;
	}


	return PACK_UNKNOWN;
}


bool recvmtr(
	const int rawsock,
	const uint8_t ttl,
	const uint reqc,
	const time_t timeout,
	struct in_addr *res, size_t *resc,
	time_t *avg
)
{
	*resc = 0; 
	time_t tott = 0; 
	bool dest = false;

	struct timespec st, ct;
	clock_gettime(CLOCK_MONOTONIC, &st);
	clock_gettime(CLOCK_MONOTONIC, &ct);


	while (*resc < reqc) {

		struct timespec dt;
		dt.tv_nsec = ct.tv_nsec - st.tv_nsec;
		dt.tv_sec = ct.tv_sec - st.tv_sec;
		const time_t dtms = dt.tv_sec * 1000 + dt.tv_nsec / 1000000;

		if (dtms > timeout) {
			break;
		}

		uint retv;
		switch (retv = recvtr(rawsock, ttl, res + *resc)) {
		case PACK_DEST:
			(*resc)++;
			tott += dtms;
			dest = true;
			break;
		case PACK_NONDEST:
			(*resc)++;
			tott += dtms;
			break;
		}

		clock_gettime(CLOCK_MONOTONIC, &ct);
	}


	if (avg != NULL && *resc != 0) {
		*avg = tott / *resc;
	}

	return dest;
}
