

#include "utils.h"


int handle(const char *desc, int retv)
{
	if (retv < 0) {
		printf("%3d - %s (%s)\n", errno, strerror(errno), desc);
		exit(1);
	}

	return retv;
}


static int addrcmp(const void *a,  const void *b)
{
	return ((struct in_addr *)a)->s_addr - ((struct in_addr *)b)->s_addr;
}


void printaddr(struct in_addr *addrs, const size_t addrc)
{

	qsort(addrs, addrc, sizeof(*addrs), addrcmp);
	
	printf("%15s", addrc == 0 ? "*" : inet_ntoa(addrs[0]));
	
	for (uint i = 1; i < addrc; i++) {
		if (addrs[i].s_addr == addrs[i - 1].s_addr) {
			continue;
		}

		printf(" | %15s", inet_ntoa(addrs[i]));
	}
}

static void printhelp()
{
	printf(
		"Usage:\n"
		"  traceroute [-t timeout ] [ -m maxttl ] [ -r requests ] addr\n"
		"Options:\n"
		"  -t timeout  | --timeout timeout\n"
		"    Set maximum waiting time for responses in miliseconds (default: 1000).\n"
		"  -m maxttl   | --maxttl maxttl\n"
		"    Set maximum TTL value (default: 30).\n"
		"  -r requests | --requests requests\n"
		"    Set number of requests for each TTL value (default: 3).\n"
	);
}

#define OPTARG(v, short, long) (strcmp(v, short) == 0 || strcmp(v, long) == 0)


static int parseopt(
	const char *arg,
	const char *val,
	struct params *result
)
{
	uint tmp;
	const int retv = sscanf(val, "%u", &tmp);

	if (OPTARG(arg, "-t", "--timeout")) {
		result->timeout = tmp;
	} else if (OPTARG(arg, "-m", "--maxttl")) {
		result->maxttl = tmp;
	} else if (OPTARG(arg, "-r", "--requests")) {
		result->reqc = tmp;
	} else {
		printf("Unknown option: %s.\n", arg);
		printf("Try with `--help` for more information.\n");
		exit(1);
	}

	return retv;
}


struct params parseargs(char **argv, const size_t argc)
{

	struct params result = { { 0 }, 1000, 30, 3 };

	bool addr = false;

	for (uint i = 0; i < argc; i++) {
		int retv = 0;

		if (OPTARG(argv[i], "-h", "--help")) {
			printhelp();
			exit(1);
		} else if (argv[i][0] == '-' && i < argc - 1) {
			retv = parseopt(argv[i], argv[i + 1], &result);
			i++;
		} else {
			retv = handle(
				"parse ip adress",
				inet_pton(AF_INET, argv[i], &result.addr.sin_addr)
			);

			addr = true;
		}


		if (retv <= 0) {
			printf("Incorrect argument: %s.\n", argv[i]);
			printf("Try with `--help` for more information.\n");
			exit(1);
		}
	}

	if (!addr) {
		printf("Remote server address not given.\n");
		exit(1);
	}

	return result;
}
