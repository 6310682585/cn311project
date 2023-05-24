#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#define SUCCESS 0
#define ERROR 1

#define END_LINE 0x0
#define SERVER_PORT 5000
#define MAX_MSG 100
#define NTHREADS_CHECK_RESULT 3

char *dataName[3];

int x = 4;
int y = 4;
int i, j;

pthread_mutex_t result_mutex;
char result = 's';

char xo[40];
int countX[3][4] = {{0, 0, 0, 0},
					{0, 0, 0, 0},
					{0, 0, 0, 0}};

int countO[3][4] = {{0, 0, 0, 0},
					{0, 0, 0, 0},
					{0, 0, 0, 0}};

/* function */
int read_line();
int random_number();
void draw_board();
void append();
void xo_board();
void count_xo();
void *result_horizon_vertical();
void *result_diagonal();
void *result_boardFull();
char check_result();
int check_valid();
int check_point();
void choose_x_block();

int main(int argc, char *argv[])
{
	char board[4][4] = {{' ', ' ', ' ', ' '},
						{' ', ' ', ' ', ' '},
						{' ', ' ', ' ', ' '},
						{' ', ' ', ' ', ' '}};

	int sd, newSd, cliLen, count, move;

	struct sockaddr_in cliAddr, servAddr;
	char line[MAX_MSG];
	char sendBuff[1025];
	char command[20];

	memset(sendBuff, '0', sizeof(sendBuff));

	/* create socket */
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
	{
		perror("cannot open socket port1 ");
		return ERROR;
	}

	/* bind server port */
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(SERVER_PORT);

	if (bind(sd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
	{
		perror("cannot bind port ");
		return ERROR;
	}

	listen(sd, 5);

	dataName[0] = "command";
	dataName[1] = "x";
	dataName[2] = "y";

	while (1)
	{

		printf(" waiting client on port TCP %u\n", SERVER_PORT);

		cliLen = sizeof(cliAddr);
		newSd = accept(sd, (struct sockaddr *)&cliAddr, &cliLen);
		if (newSd < 0)
		{
			perror("cannot accept connection ");
			return ERROR;
		}

		/* init line */
		memset(line, 0x0, MAX_MSG);

		/* receive segments */
		count = 0;
		while (read_line(newSd, line) != ERROR)
		{

			printf("%s: received from %s:TCP%d : %s is %s\n", argv[0],
				   inet_ntoa(cliAddr.sin_addr),
				   ntohs(cliAddr.sin_port),
				   dataName[count], line);
			count += 1;

			if (count == 1)
			{
				strcpy(command, line);
				if (strcmp(command, "start") == 0)
				{
					result = 's';
					break;
				}
			}
			if (count == 2)
			{
				sscanf(line, "%d", &x);
			}
			if (count == 3)
			{
				sscanf(line, "%d", &y);
				break;
			}

			/* init line */
			memset(line, 0x0, MAX_MSG);

		} /* while(read_line) */

		if (strcmp(command, "move") == 0)
		{
			if (check_point(x, y) == 0)
			{
				snprintf(sendBuff, sizeof(sendBuff), " (%d, %d) selected point invalid, please try again\n", x, y);
				write(newSd, sendBuff, strlen(sendBuff));
				close(newSd);
			}
			else
			{
				check_result(board);
				if (result == 'w' || result == 'l' || result == 'f')
				{
					strcpy(xo, "");
					append(xo, 't');
					xo_board(board);
					append(xo, result);
					snprintf(sendBuff, sizeof(sendBuff), "%s", xo);
					write(newSd, sendBuff, strlen(sendBuff));
					close(newSd);
				}
				else
				{
					if (check_valid(board[x][y]) == 1)
					{
						printf("client move :\n");
						board[x][y] = 'O';
						draw_board(board);
						strcpy(xo, "");
						append(xo, 'b');
						xo_board(board);

						check_result(board);
						if (result == 'w')
						{
							append(xo, 'w');
						}
						else
						{
							printf("sever move :\n");
							choose_x_block(board);
							draw_board(board);
							check_result(board);
							append(xo, result);
							append(xo, 'b');
							xo_board(board);
						}

						snprintf(sendBuff, sizeof(sendBuff), "%s", xo);
						write(newSd, sendBuff, strlen(sendBuff));
						close(newSd);
					}
					else
					{
						snprintf(sendBuff, sizeof(sendBuff), " this block had been selected, please choose another block\n");
						write(newSd, sendBuff, strlen(sendBuff));
						close(newSd);
					}
				}
			}
		}

		else if (strcmp(command, "start") == 0)
		{
			printf(" here start \n");
			for (i = 0; i <= 3; i++)
			{
				for (j = 0; j <= 3; j++)
				{
					board[i][j] = ' ';
				}
			}
			draw_board(board);
			snprintf(sendBuff, sizeof(sendBuff), " start game! choose your move\n");
			write(newSd, sendBuff, strlen(sendBuff));
			close(newSd);
		}

	} /* while (1) */
}

int read_line(int newSd, char *line_to_return)
{

	static int rcv_ptr = 0;
	static char rcv_msg[MAX_MSG];
	static int n;
	int offset;

	offset = 0;

	while (1)
	{
		if (rcv_ptr == 0)
		{
			/* read data from socket */
			memset(rcv_msg, 0x0, MAX_MSG);		  /* init buffer */
			n = recv(newSd, rcv_msg, MAX_MSG, 0); /* wait for data */
			if (n < 0)
			{
				perror(" cannot receive data ");
				return ERROR;
			}
			else if (n == 0)
			{
				printf(" connection closed by client\n");
				close(newSd);
				return ERROR;
			}
		}

		/* if new data read on socket */
		/* OR */
		/* if another line is still in buffer */

		/* copy line into 'line_to_return' */
		while (*(rcv_msg + rcv_ptr) != END_LINE && rcv_ptr < n)
		{
			memcpy(line_to_return + offset, rcv_msg + rcv_ptr, 1);
			offset++;
			rcv_ptr++;
		}

		/* end of line + end of buffer => return line */
		if (rcv_ptr == n - 1)
		{
			/* set last byte to END_LINE */
			*(line_to_return + offset) = END_LINE;
			rcv_ptr = 0;
			return ++offset;
		}

		/* end of line but still some data in buffer => return line */
		if (rcv_ptr < n - 1)
		{
			/* set last byte to END_LINE */
			*(line_to_return + offset) = END_LINE;
			rcv_ptr++;
			return ++offset;
		}

		/* end of buffer but line is not ended => */
		/*  wait for more data to arrive on socket */
		if (rcv_ptr == n)
		{
			rcv_ptr = 0;
		}

	} /* while */
}

int random_number(int size)
{
	int result;

	time_t now = time(NULL);
	struct tm *tm_struct = localtime(&now);

	int sec = tm_struct->tm_sec;

	for (i = 1; i <= sec; i++)
	{
		result = rand();
	}

	return result % size;
}

void draw_board(char board[][4])
{
	printf(" %c | %c | %c | %c \n", board[0][0], board[0][1], board[0][2], board[0][3]);
	printf("---------------\n");
	printf(" %c | %c | %c | %c \n", board[1][0], board[1][1], board[1][2], board[1][3]);
	printf("---------------\n");
	printf(" %c | %c | %c | %c \n", board[2][0], board[2][1], board[2][2], board[2][3]);
	printf("---------------\n");
	printf(" %c | %c | %c | %c \n", board[3][0], board[3][1], board[3][2], board[3][3]);
}

void append(char *s, char c)
{
	int len = strlen(s);
	s[len] = c;
	s[len + 1] = '\0';
}

void xo_board(char board[][4])
{
	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 3; j++)
		{
			append(xo, board[i][j]);
		}
	}
}

void count_xo(char board[][4])
{
	int X_vertical = 0;
	int O_vertical = 0;
	int X_horizon = 0;
	int O_horizon = 0;
	int X_diagonal1 = 0;
	int O_diagonal1 = 0;
	int X_diagonal2 = 0;
	int O_diagonal2 = 0;

	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 3; j++)
		{
			if (board[i][j] == 'X')
			{
				X_horizon += 1;
			}
			else if (board[i][j] == 'O')
			{
				O_horizon += 1;
			}

			if (board[j][i] == 'X')
			{
				X_vertical += 1;
			}
			else if (board[j][i] == 'O')
			{
				O_vertical += 1;
			}

			if (i == j)
			{
				if (board[i][j] == 'X')
				{
					X_diagonal1 += 1;
				}
				else if (board[i][j] == 'O')
				{
					O_diagonal1 += 1;
				}
			}
			if (i + j == 3)
			{
				if (board[i][j] == 'X')
				{
					X_diagonal2 += 1;
				}
				else if (board[i][j] == 'O')
				{
					O_diagonal2 += 1;
				}
			}
		}

		countX[0][i] = X_horizon;
		countX[1][i] = X_vertical;
		countO[0][i] = O_horizon;
		countO[1][i] = O_vertical;

		X_vertical = 0;
		O_vertical = 0;
		X_horizon = 0;
		O_horizon = 0;
	}

	countX[2][1] = X_diagonal1;
	countX[2][2] = X_diagonal2;
	countO[2][1] = O_diagonal1;
	countO[2][2] = O_diagonal2;
}

void *result_horizon_vertical(void *arg) 
{
	for (int i = 0; i <= 1; i++)
	{
		for (int j = 0; j <= 3; j++)
		{
			if (countX[i][j] == 4)
			{
				pthread_mutex_lock(&result_mutex);
				if(result == 'c' || result == 's') {
					printf("client lose!\n");
					result = 'l';
				}
				pthread_mutex_unlock(&result_mutex);
    			pthread_exit(NULL);
			}
			if (countO[i][j] == 4)
			{
				pthread_mutex_lock(&result_mutex);
				if(result == 'c' || result == 's') {
					printf("client win!\n");
					result = 'w';
				}
				pthread_mutex_unlock(&result_mutex);
    			pthread_exit(NULL);
			}
		}
	}
}

void *result_diagonal(void *arg) 
{
	if (countX[2][1] == 4 || countX[2][2] == 4)
	{
		pthread_mutex_lock(&result_mutex);
		if(result == 'c' || result == 's') {
			printf("client lose!\n");
			result = 'l';
		}
		pthread_mutex_unlock(&result_mutex);
    	pthread_exit(NULL);
	}
	if (countO[2][1] == 4 || countO[2][2] == 4)
	{
		pthread_mutex_lock(&result_mutex);
		if(result == 'c' || result == 's') {
			printf("client win!\n");
			result = 'w';
		}
		pthread_mutex_unlock(&result_mutex);
    	pthread_exit(NULL);
	}
}

void *result_boardFull(void *arg) 
{
	int count = countX[0][0] + countX[0][1] + countX[0][2] + countX[0][3];
	count += countO[0][0] + countO[0][1] + countO[0][2] + countO[0][3];
	if (count == 16)
	{
		pthread_mutex_lock(&result_mutex);
		if(result == 'c' || result == 's') {
			printf("board full\n");
			result = 'f';
		}
		pthread_mutex_unlock(&result_mutex);
    	pthread_exit(NULL);
	}
	pthread_mutex_lock(&result_mutex);
	if(result == 'c' || result == 's') {
		result = 'c';
	}
	pthread_mutex_unlock(&result_mutex);
    pthread_exit(NULL);
}

char check_result(char board[][4])
{
	count_xo(board);

	int start, tids[NTHREADS_CHECK_RESULT];
    pthread_t threads[NTHREADS_CHECK_RESULT];
    pthread_attr_t attr;

	pthread_mutex_init(&result_mutex, NULL);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (int i = 0; i < NTHREADS_CHECK_RESULT; i++)
    {
        tids[i] = i;
        if (i == 0)
        {
            pthread_create(&threads[i], &attr, result_horizon_vertical, NULL);
        }
        else if (i == 1)
        {
            pthread_create(&threads[i], &attr, result_diagonal, NULL);
        }
        else if (i == 2)
        {
            pthread_create(&threads[i], &attr, result_boardFull, NULL);
        }
    }

    for (int i = 0; i < NTHREADS_CHECK_RESULT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* Clean up and exit */
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&result_mutex);
}

int check_valid(char point)
{
	if (point == 'X' || point == 'O')
	{
		return 0;
	}
	return 1;
}

int check_point(int x, int y)
{
	if (x >= 0 && x <= 3)
	{
		if (y >= 0 && y <= 3)
		{
			return 1;
		}
	}
	return 0;
}

void choose_x_block(char board[][4])
{
	count_xo(board);
	int count = countX[0][0] + countX[0][1] + countX[0][2] + countX[0][3];
	count += countO[0][0] + countO[0][1] + countO[0][2] + countO[0][3];
	count = 16 - count;

	int block = random_number(count);
	// printf("%d %d %d %d, %d %d %d %d\n", countX[0][0], countX[0][1], countX[0][2], countX[0][3], countO[0][0], countO[0][1], countO[0][2], countO[0][3]);
	// printf("block: %d count: %d\n", block, count);
	count = 0;
	for (i = 0; i <= 3; i++)
	{
		for (j = 0; j <= 3; j++)
		{
			if (check_valid(board[i][j]) == 1)
			{
				if (block == count)
				{
					board[i][j] = 'X';
					count += 1;
				}
				else
				{
					count += 1;
				}
			}
		}
	}
}