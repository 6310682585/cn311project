/* fpont 12/99 */
/* pont.net    */
/* tcpClient.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h>

#define SERVER_PORT 5000
#define MAX_MSG 100

char *dataName[3];

int x = 4;
int y = 4;
char xChar[1];
char yChar[1];

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

int main(int argc, char *argv[])
{
  char board[4][4] = {{'T', ' ', ' ', ' '},
                      {' ', ' ', ' ', ' '},
                      {' ', ' ', ' ', ' '},
                      {' ', ' ', ' ', ' '}};
  int sd, rc, i, j, n = 0;
  struct sockaddr_in localAddr, servAddr;
  struct hostent *h;
  char recvBuff[1024];

  memset(recvBuff, '0', sizeof(recvBuff));

  if (argc < 3)
  {
    printf("usage: %s <server> <command> <x> <y>\n", argv[0]);
    exit(1);
  }

  h = gethostbyname(argv[1]);
  if (h == NULL)
  {
    printf("%s: unknown host '%s'\n", argv[0], argv[1]);
    exit(1);
  }

  servAddr.sin_family = h->h_addrtype;
  memcpy((char *)&servAddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
  servAddr.sin_port = htons(SERVER_PORT);

  /* create socket */
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0)
  {
    perror("cannot open socket ");
    exit(1);
  }

  /* bind any port number */
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  localAddr.sin_port = htons(0);

  rc = bind(sd, (struct sockaddr *)&localAddr, sizeof(localAddr));
  if (rc < 0)
  {
    printf("%s: cannot bind port TCP %u\n", argv[0], SERVER_PORT);
    perror("error ");
    exit(1);
  }

  /* connect to server */
  rc = connect(sd, (struct sockaddr *)&servAddr, sizeof(servAddr));
  if (rc < 0)
  {
    perror("cannot connect ");
    exit(1);
  }

  dataName[0] = "command";
  dataName[1] = "x";
  dataName[2] = "y";

  for (i = 2; i < argc; i++)
  {

    rc = send(sd, argv[i], strlen(argv[i]) + 1, 0);

    if (rc < 0)
    {
      perror("cannot send data ");
      close(sd);
      exit(1);
    }
  }

  while ((n = read(sd, recvBuff, sizeof(recvBuff) - 1)) > 0)
  {
    // printf("%s\n", recvBuff);
    if (recvBuff[0] == ' ')
    {
      recvBuff[n] = 0;
      if (fputs(recvBuff, stdout) == EOF)
      {
        printf("\n Error : Fputs error\n");
      }

      if (recvBuff[1] == 's')
      {
        for (i = 0; i <= 3; i++)
        {
          for (j = 0; j <= 3; i++)
          {
            board[i][j] = ' ';
          }
        }
        draw_board(board);
      }
    }
    else if (recvBuff[0] == 'b' || recvBuff[0] == 't')
    {
      for (i = 0; i <= 15; i++)
      {
        x = i / 4;
        y = i % 4;
        board[x][y] = recvBuff[i + 1];
      }
      if (recvBuff[0] == 'b')
      {
        printf("your move :\n");
      }
      draw_board(board);

      if (recvBuff[18] == 'b')
      {
        for (i = 0; i <= 15; i++)
        {
          x = i / 4;
          y = i % 4;
          board[x][y] = recvBuff[i + 19];
        }
        printf("\nsever move :\n");
        draw_board(board);
      }

      if (recvBuff[17] == 'c')
      {
        printf("\nyour turn!\n\n");
      }
      else if (recvBuff[17] == 'l')
      {
        printf("\nyour lose!\n\n");
      }
      else if (recvBuff[17] == 'w')
      {
        printf("\nyour win!\n");
      }
      else if (recvBuff[17] == 'f')
      {
        printf("\nboard full please start again for retry\n");
      }
    }
  }

  if (n < 0)
  {
    printf("\n Read error sd \n");
  }

  return 0;
}
