void tracer(
	const int rawsock,
	const struct sockaddr_in addr,
	const time_t timeout,
	const uint8_t maxttl,
	const uint reqc
)
{
	size_t resc;
	struct in_addr pool[reqc];

	for (uint8_t ttl = 1; ttl <= maxttl; ttl++) {

		sendmtr(rawsock, addr, ttl, reqc);

		time_t avgt;
		bool dest = recvmtr(rawsock, ttl, reqc, timeout, pool, &resc, &avgt);

		printf("  %c ", dest ? 'D' : '#');
		printaddr(pool, resc);
		printf(" ");

		if (resc != reqc) {
			printf("(""???"")");
		} else {
			printf("(%lu ms)", avgt);
		}

		printf("\n");

		if (dest) {
			break;
		}
	}
}

int main(int argc, char **argv)
{
	struct params prms = parseargs(argv + 1, argc - 1);

	int rawsock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	handle("create raw socket", rawsock);

	tracer(rawsock, prms.addr, prms.timeout, prms.maxttl, prms.reqc);

	return 0;
}
