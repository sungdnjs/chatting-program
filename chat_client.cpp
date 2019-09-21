#include <pthread.h>
#include <unistd.h> 
#include <sys/stat.h>
#include <arpa/inet.h>
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>

#define MAXLINE 1024
#define PORTNUM 3500 

void * send_msg(void *data)
{
	int sockfd = *((int *)data);
	char buf[MAXLINE];
	memset(buf, 0x00, MAXLINE);

	while (1)
	{
		read(0, buf, MAXLINE); //0�� ���ϵ�ũ���� �ڸ��ε� 0���� �����ϴϱ� �����Լ� �Ἥ Ű�����Է��� �޴°���

		if (write(sockfd, buf, MAXLINE) <= 0)
		{
			perror("write error : ");
			exit(1);
		}

		memset(buf, 0x00, MAXLINE); //�������ϱ� ����
	}

}


int main(int argc, char *argv[])
{
	struct sockaddr_in serveraddr;
	int server_sockfd;
	int client_len;
	char buf[MAXLINE];//������ ����
	char rbuf[MAXLINE];//�޴� ����
	pthread_t thread_id;

	if (argc < 3)
	{
		fprintf(stderr, "Usage: %s <ip address> <chat id>\n", argv[0]);
		exit(1);
	}

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("error :");
		return 1;
	}
	server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(PORTNUM);
	client_len = sizeof(serveraddr);

	if (connect(server_sockfd, (struct sockaddr *)&serveraddr, client_len) < 0)
	{
		perror("connect error :");
		return 1;
	}

	//Ŀ��Ʈ �Ǿ����� ���ӵȰŴϱ� �°� ȯ���ϰ� ���̵� �Ѱ��ֱ�.
	printf("%s�� ��Ƽ ä�ù濡 ���� ���� ȯ���մϴ�!\n", argv[2]);
	memset(buf, 0x00, MAXLINE);
	strcat(buf, argv[2]);
	//�� ���̵� ������
	if (write(server_sockfd, buf, MAXLINE) <= 0)
	{
		perror("write error : ");
		return 1;
	}

	memset(buf, 0x00, MAXLINE);

	//�ڽ� ������� �޼��� ������
	pthread_create(&thread_id, NULL, send_msg, (void *)&server_sockfd);

	//�θ� ������� �޼��� �о����
	while (1)
	{
		if (read(server_sockfd, buf, MAXLINE) <= 0)
		{
			return 1;
		}
		printf("%s", buf);
	}
	pthread_detach(thread_id);
	close(server_sockfd);

	return 0;
}