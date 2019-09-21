#include <pthread.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#define MAXLINE 1024 
#define PORTNUM 3500

int client_num = 0;//클라이언트 몇명인지
int client_sock_arr[10];//클라이언트들의 소켓 저장
char client_id_arr[5][32];//클라이언트 아이디 저장
pthread_mutex_t mutx;//뮤텍스


void send_msg(char * buf, int len, int sockfd)//메세지를 나빼고 모든 클라이언트들에게 보내기
{
	int j;
	pthread_mutex_lock(&mutx);

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
			j = i;
	}

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd != client_sock_arr[i])//내가 아닐때만 메세지 보내기
		{
			write(client_sock_arr[i], client_id_arr[j], len);
			write(client_sock_arr[i], " : ", len);
			write(client_sock_arr[i], buf, len);
		}
	}
	pthread_mutex_unlock(&mutx);

	memset(buf, 0x00, MAXLINE);

}

void id_msg(int len, int sockfd)//입장했을 때 메세지 보내기
{
	int j;
	pthread_mutex_lock(&mutx);
	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
			j = i;
	}

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd != client_sock_arr[i])
		{
			write(client_sock_arr[i], client_id_arr[j], len);
			write(client_sock_arr[i], "님이 대화방에 입장하셨습니다.\n", len);

		}
	}
	pthread_mutex_unlock(&mutx);

}

void exit_msg(int len, int sockfd)//퇴장 메세지
{
	int j;
	pthread_mutex_lock(&mutx);
	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
			j = i;
	}
	write(client_sock_arr[j], "채팅이 종료되었습니다.\n", len);//나한테 보내는거

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd != client_sock_arr[i])//나 아닐때
		{
			write(client_sock_arr[i], client_id_arr[j], len);
			write(client_sock_arr[i], "님이 대화방을 나가셨습니다.\n", len);

		}
	}
	pthread_mutex_unlock(&mutx);

}

void show(int len, int sockfd)//현재 누가 있나 보여주기
{

	pthread_mutex_lock(&mutx);

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
		{
			write(client_sock_arr[i], "현재 접속중인 사용자는 아래와 같습니다.\n", len);
			for (int j = 0; j < client_num; j++)
			{
				write(client_sock_arr[i], client_id_arr[j], len);
				if (j != (client_num - 1))
					write(client_sock_arr[i], ", ", len);
			}
			write(client_sock_arr[i], "\n", len);
		}

	}
	pthread_mutex_unlock(&mutx);

}

void * client_connect(void *data) //클라이언트들 연결
{

	int sockfd = *((int *)data);
	int readn = 0;
	char id[32];
	socklen_t addrlen;
	char buf[MAXLINE];
	struct sockaddr_in client_addr;
	int i;

	memset(buf, 0x00, MAXLINE);
	addrlen = sizeof(client_addr);
	getpeername(sockfd, (struct sockaddr *)&client_addr, &addrlen);//어디에서 접속했는지 알려고 getpeername을 씀

	pthread_mutex_lock(&mutx);

	readn = read(sockfd, id, MAXLINE);//아이디 읽기

	for (i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
		{
			for (int j = 0; j < 32; j++)
			{
				client_id_arr[i][j] = id[j];
			}
		}
	}
	pthread_mutex_unlock(&mutx);


	if (readn > 0)
	{
		id_msg(readn, sockfd);//새로 접속한 아이디 보내주기
	}
	while ((readn = read(sockfd, buf, MAXLINE)) > 0)
	{ //쓰레드 함수내에서 클라이언트에서 보내준거 리드해서 화면에 찍고 돌려주기

		if (strcmp(buf, "@show\n") == 0)
		{
			show(readn, sockfd);
		}
		else if (strcmp(buf, "@exit\n") == 0)//클라이언트 연결 종료
		{
			exit_msg(readn, sockfd);
			break;
		}
		else
		{
			send_msg(buf, readn, sockfd);
		}
		//printf("Read Data %s(%d) : %s", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), buf);
		//write(sockfd, buf, strlen(buf));
		memset(buf, 0x00, MAXLINE);
	}
	//클라이언트 연결 종료

	pthread_mutex_lock(&mutx);

	for (int i; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
		{
			for (; i < client_num - 1; i++)
			{
				client_sock_arr[i] = client_sock_arr[i + 1];//클라이언트 파일 디스크립터 지우기
				for (int j = 0; j < 32; j++)
				{
					client_id_arr[i][j] = client_id_arr[i + 1][j];//클라이언트 이름 지우기
				}
			}
			break;
		}
	}

	client_num--;//클라이언수 수 --
	pthread_mutex_unlock(&mutx);

	close(sockfd);
	printf("worker thread end\n");
	return 0;
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd;//서버와 클라이언트 소켓 생성시 반환되는 파일 디스크립터 저장
	struct sockaddr_in server_addr, client_addr;//주소 저장할 구조체
	socklen_t addrlen;//클라이언트의 주소 크기 저장
	char buf[MAXLINE];
	pthread_t thread_id;

	if (pthread_mutex_init(&mutx, NULL) != 0)//뮤택스 초기화
	{
		perror("Mutex Init failure");
		return 1;
	}

	if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return 1;
	}
	memset((void *)&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORTNUM);

	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind error");
		return 1;
	}

	if (listen(listen_fd, 5) == -1)
	{
		perror("listen error"); return 1;
	}

	while (1)
	{
		addrlen = sizeof(client_addr);
		client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addrlen);
		if (client_fd == -1)
		{
			printf("accept error\n");
		}
		else
		{
			pthread_mutex_lock(&mutx);
			client_sock_arr[client_num++] = client_fd;
			pthread_mutex_unlock(&mutx);

			pthread_create(&thread_id, NULL, client_connect, (void *)&client_fd);//스레드 만들기
			pthread_detach(thread_id);// 워커 스레드랑 메인스레드랑 분리해서 종료되면 알아서 자원회수
		}
	}
	return 0;
}