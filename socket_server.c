#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void start_server(char *filename, int port)
{
	int			sockfd = 0;
	int			new_sockfd = 0;
	char			buffer[1024 * 1024] = {0, };
	struct sockaddr_in	addr;
	int			fd = 0;
	int			ret = 0;
	int			addr_len = 0;

	addr_len = sizeof(struct sockaddr_in);
	bzero(&addr, sizeof(addr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket");
		exit (0);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		exit(1);
	}

	if (listen(sockfd, 3) < 0) {
		perror("listen");
		exit(1);
	}

	while (1) {
		if ((new_sockfd = accept(sockfd, 
						(struct sockaddr *)&addr, 
						(socklen_t *)&addr_len)) < 0) {
			perror("accept");
			exit(0);
		}
		write(new_sockfd, "OK", strlen("OK"));
		
		fd = open(filename, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG |S_IRWXO );
		while ((ret = recv(new_sockfd, buffer, 1024 * 1024, 0))) {
			write(fd, buffer, ret);
		}
		close(fd);
	}

}

void usage(void)
{
	fprintf(stderr, "./test [-f file path] [-p listen port]\n");
}

int main(int argc, char *argv[])
{
	int	ret = -1;
	int	opt = 0;
	int	port = 12365;
	char	filename[512] = {0, };
	char	ipaddr[32] = {0, };

	fprintf(stdout, "In programe\n");

	strncpy(ipaddr, "127.0.0.1", strlen("127.0.0.1"));

	if (argc < 2) {
		usage();
		return ret;
	}

	while ((opt = getopt(argc, argv, "Hp:f:")) != -1) {
		switch (opt) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'f':
				memset(filename, 0, sizeof(filename));
				strncpy(filename, optarg, strlen(optarg));
				break;
			default:
				usage();
				break;
		}
	}



	start_server(filename, port);
	fprintf(stdout, "Out programe\n");
	return ret;
}
