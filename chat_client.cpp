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
		read(0, buf, MAXLINE); //0이 파일디스크립터 자리인데 0으로 리드하니까 리드함수 써서 키보드입력을 받는거지

		if (write(sockfd, buf, MAXLINE) <= 0)
		{
			perror("write error : ");
			exit(1);
		}

		memset(buf, 0x00, MAXLINE); //보냈으니까 지워
	}

}


int main(int argc, char *argv[])
{
	struct sockaddr_in serveraddr;
	int server_sockfd;
	int client_len;
	char buf[MAXLINE];//보내는 버퍼
	char rbuf[MAXLINE];//받는 버퍼
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

	//커넥트 되었으면 접속된거니까 온거 환영하고 아이디 넘겨주기.
	printf("%s님 멀티 채팅방에 오신 것을 환영합니다!\n", argv[2]);
	memset(buf, 0x00, MAXLINE);
	strcat(buf, argv[2]);
	//내 아이디 보내기
	if (write(server_sockfd, buf, MAXLINE) <= 0)
	{
		perror("write error : ");
		return 1;
	}

	memset(buf, 0x00, MAXLINE);

	//자식 스레드는 메세지 보내기
	pthread_create(&thread_id, NULL, send_msg, (void *)&server_sockfd);

	//부모 스레드는 메세지 읽어오기
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