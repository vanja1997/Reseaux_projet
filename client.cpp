#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <sys/timeb.h>
#include <time.h>


typedef struct Player{
  char Nom[100];
  char score[1];
  char coup[7];
}Player;

void
Client_send (int fd, const char *buf, int len)
{
  int sent_len = 0;
  int n;
  
  while (sent_len < len) {
    n = write(fd, buf + sent_len, len - sent_len);
    if (n < 0) {
      break;
    }
    sent_len += n;
  }
  
  if (n == -1) {
    fprintf(stderr, "Server_send: partial send\n");
  }
}

void
Client_sendf (int fd, const char *fmt, ...)
{
  va_list va;
  char buf[4096];
  int len;
  
  va_start(va, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, va);
  va_end(va);
  
  if (len >= (int)sizeof(buf)) {
    fprintf(stderr, "Server_sendf: string too long, truncated\n");
  }
  
  Client_send(fd, buf, len);
}

void numerotation(char coup[])
{
  if(strncmp(coup,"0",1)==0)
    strncpy(coup,"PIERRE",6);
  else if(strncmp(coup,"1",1)==0)
    strncpy(coup,"FEUILLE",7);
  else if(strncmp(coup,"2",1)==0)
    strncpy(coup,"CISEAUX",7); 
}



void extraire(char buf[],int begin, char end, char player[]){
  
  int compt=0;
  for(int i=begin;buf[i]!= end;i++){
    player[compt]=buf[i];
    compt++;
    
  }
}

char vec(char ut[]){
  return ut[0];
  
}

void client (int argc, char *argv[]){
  
  Player lesplayers[2];
  int sock, peer;
  struct sockaddr_in addr;
  struct hostent *host;
  struct pollfd pfd[2];
  int n;
  char buf[512];
  int findejeu=0;
  if (argc == 3) {
    if ((host = gethostbyname(argv[1])) == NULL) {
      herror("gethostbyname");
      exit(1);
    }
    
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *((uint32_t *) host->h_addr);
    addr.sin_port = htons(atoi(argv[2]));
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      perror("socket");
      exit(1);
    }
    
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      perror("connect");
      exit(1);
    }
    
    pfd[0].fd = sock;
    pfd[0].events = POLLIN;
    pfd[1].fd = STDIN_FILENO;
    pfd[1].events = POLLIN;
    
    
    
    printf("Qui es tu ? \n");
    n=read(STDIN_FILENO,buf,sizeof(buf));
    strncpy(lesplayers[0].Nom,buf,n-1);
    Client_sendf(pfd[0].fd,"je suis %s\n",buf);
    poll(pfd,1,10000);
    n=read(pfd[0].fd,buf,sizeof(buf));
    write(STDOUT_FILENO,buf,n);
    if (strncmp(buf,"Tu es bien connecte.",20)==0){
      while(findejeu==0){
	poll(pfd,1,100000);
	n = read(pfd[0].fd,buf,sizeof(buf));
	if (strncmp(buf,"C'est partie.",13) == 0){
          printf("Choisissez un coup entre PIERRE, FEUILLE ou CISEAUX\n");
          n = read (STDIN_FILENO, buf, sizeof(buf));
          strncpy(lesplayers[0].score,buf,n);
          Client_sendf(pfd[0].fd,buf,n);
          poll(pfd,1,10000);
          read(pfd[0].fd,buf,sizeof(buf));
          if(strncmp(buf,"score/coup",8)== 0){
            extraire(buf,11,'-',lesplayers[0].score);
            extraire(buf,13,'/',lesplayers[1].score);
            extraire(buf,15,'-',lesplayers[1].coup);
            extraire(buf,17,'\n',lesplayers[1].Nom);
	    
            numerotation(lesplayers[1].coup);
            Client_sendf(STDOUT_FILENO, "%s : %c \n %s : %c\n", lesplayers[0].Nom,vec(lesplayers[0].score), lesplayers[1].Nom, vec(lesplayers[1].score) );
          }
        }
	if(strncmp(buf,"Le gagnant est",14)==0){
	  Client_send(STDOUT_FILENO,buf,n);
	  findejeu=1;
	}      
      }
    }
  }
}


int main(int argc, char *argv[]){
  
  client(argc, argv);
  return 0;  
}
