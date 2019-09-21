#include <pthread.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#define MAXLINE 1024 
#define PORTNUM 3500

int client_num = 0;//Ŭ���̾�Ʈ �������
int client_sock_arr[10];//Ŭ���̾�Ʈ���� ���� ����
char client_id_arr[5][32];//Ŭ���̾�Ʈ ���̵� ����
pthread_mutex_t mutx;//���ؽ�


void send_msg(char * buf, int len, int sockfd)//�޼����� ������ ��� Ŭ���̾�Ʈ�鿡�� ������
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
		if (sockfd != client_sock_arr[i])//���� �ƴҶ��� �޼��� ������
		{
			write(client_sock_arr[i], client_id_arr[j], len);
			write(client_sock_arr[i], " : ", len);
			write(client_sock_arr[i], buf, len);
		}
	}
	pthread_mutex_unlock(&mutx);

	memset(buf, 0x00, MAXLINE);

}

void id_msg(int len, int sockfd)//�������� �� �޼��� ������
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
			write(client_sock_arr[i], "���� ��ȭ�濡 �����ϼ̽��ϴ�.\n", len);

		}
	}
	pthread_mutex_unlock(&mutx);

}

void exit_msg(int len, int sockfd)//���� �޼���
{
	int j;
	pthread_mutex_lock(&mutx);
	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
			j = i;
	}
	write(client_sock_arr[j], "ä���� ����Ǿ����ϴ�.\n", len);//������ �����°�

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd != client_sock_arr[i])//�� �ƴҶ�
		{
			write(client_sock_arr[i], client_id_arr[j], len);
			write(client_sock_arr[i], "���� ��ȭ���� �����̽��ϴ�.\n", len);

		}
	}
	pthread_mutex_unlock(&mutx);

}

void show(int len, int sockfd)//���� ���� �ֳ� �����ֱ�
{

	pthread_mutex_lock(&mutx);

	for (int i = 0; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
		{
			write(client_sock_arr[i], "���� �������� ����ڴ� �Ʒ��� �����ϴ�.\n", len);
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

void * client_connect(void *data) //Ŭ���̾�Ʈ�� ����
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
	getpeername(sockfd, (struct sockaddr *)&client_addr, &addrlen);//��𿡼� �����ߴ��� �˷��� getpeername�� ��

	pthread_mutex_lock(&mutx);

	readn = read(sockfd, id, MAXLINE);//���̵� �б�

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
		id_msg(readn, sockfd);//���� ������ ���̵� �����ֱ�
	}
	while ((readn = read(sockfd, buf, MAXLINE)) > 0)
	{ //������ �Լ������� Ŭ���̾�Ʈ���� �����ذ� �����ؼ� ȭ�鿡 ��� �����ֱ�

		if (strcmp(buf, "@show\n") == 0)
		{
			show(readn, sockfd);
		}
		else if (strcmp(buf, "@exit\n") == 0)//Ŭ���̾�Ʈ ���� ����
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
	//Ŭ���̾�Ʈ ���� ����

	pthread_mutex_lock(&mutx);

	for (int i; i < client_num; i++)
	{
		if (sockfd == client_sock_arr[i])
		{
			for (; i < client_num - 1; i++)
			{
				client_sock_arr[i] = client_sock_arr[i + 1];//Ŭ���̾�Ʈ ���� ��ũ���� �����
				for (int j = 0; j < 32; j++)
				{
					client_id_arr[i][j] = client_id_arr[i + 1][j];//Ŭ���̾�Ʈ �̸� �����
				}
			}
			break;
		}
	}

	client_num--;//Ŭ���̾�� �� --
	pthread_mutex_unlock(&mutx);

	close(sockfd);
	printf("worker thread end\n");
	return 0;
}

int main(int argc, char **argv)
{
	int listen_fd, client_fd;//������ Ŭ���̾�Ʈ ���� ������ ��ȯ�Ǵ� ���� ��ũ���� ����
	struct sockaddr_in server_addr, client_addr;//�ּ� ������ ����ü
	socklen_t addrlen;//Ŭ���̾�Ʈ�� �ּ� ũ�� ����
	char buf[MAXLINE];
	pthread_t thread_id;

	if (pthread_mutex_init(&mutx, NULL) != 0)//���ý� �ʱ�ȭ
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

			pthread_create(&thread_id, NULL, client_connect, (void *)&client_fd);//������ �����
			pthread_detach(thread_id);// ��Ŀ ������� ���ν������ �и��ؼ� ����Ǹ� �˾Ƽ� �ڿ�ȸ��
		}
	}
	return 0;
}