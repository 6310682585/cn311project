#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>
#include <sys/types.h>

#define SUCCESS 0
#define ERROR 1

#define END_LINE 0x0
#define SERVER_PORT 12585
#define MAX_MSG 100

char *dataName[3];

int x = 4;
int y = 4;
int i, j;

char xo[40];

/* function readline */
int read_line();

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

int main(int argc, char *argv[])
{
  char board[4][4] = {{'T', ' ', ' ', ' '},
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

    printf("end while read\n");

    if (strcmp(command, "move") == 0)
    {
      printf("client move :\n");
      board[x][y] = 'O';
      draw_board(board);
      strcpy(xo, "");
      xo_board(board);
      append(xo, 'c');

      printf("\nsever move :\n");
      board[0][0] = 'x';
      draw_board(board);
      xo_board(board);

      snprintf(sendBuff, sizeof(sendBuff), xo);
      write(newSd, sendBuff, strlen(sendBuff));
      close(newSd);
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
      snprintf(sendBuff, sizeof(sendBuff), "start");
      write(newSd, sendBuff, strlen(sendBuff));
      close(newSd);
    }

  } /* while (1) */
}

/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING       */
/* this function is experimental.. I don't know yet if it works  */
/* correctly or not. Use Steven's readline() function to have    */
/* something robust.                                             */
/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING       */

/* rcv_line is my function readline(). Data is read from the socket when */
/* needed, but not byte after bytes. All the received data is read.      */
/* This means only one call to recv(), instead of one call for           */
/* each received byte.                                                   */
/* You can set END_CHAR to whatever means endofline for you. (0x0A is \n)*/
/* read_lin returns the number of bytes returned in line_to_return       */
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
      memset(rcv_msg, 0x0, MAX_MSG);        /* init buffer */
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
