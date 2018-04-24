#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <time.h>

typedef struct Player{
  char Nom[100];
  int score;
  int coup;
}Player;


void
Server_send (int fd, const char *buf, int len)
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
Server_sendf (int fd, const char *fmt, ...)
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
  
  Server_send(fd, buf, len);
}

void
Server_broadcast (struct pollfd *fds, const char *buf, int len)
{
  int i;
  
  for (i = 1; i <= 2; i++) {
    Server_send(fds[i].fd, buf, len);
  }
}

void
Server_broadcastf (struct pollfd *fds, const char *fmt, ...)
{
  va_list va;
  char buf[4096];
  int len;
  
  va_start(va, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, va);
  va_end(va);
  
  if (len >= (int)sizeof(buf)) {
    fprintf(stderr, "Server_broadcastf: string too long, truncated\n");
  }
  
  Server_broadcast(fds, buf, len);
}

void extraire(char buf[],int begin, char end, char player[]){
  
  int compt=0;
  for(int i=begin;buf[i]!= end;i++){
    player[compt]=buf[i];
    compt++;
    
  }
}

void
Client_accept(struct pollfd pfd[],int sock,Player lesplayers[])
{
  int n=0;
  char buf[512];
  int peer;
  for(int i=0 ; i < 2; i++){
    if((peer = accept(sock,NULL,NULL)) < 0){
      perror("accept");
      exit(1);
    }
    pfd[i].fd = peer;
    pfd[i].events = POLLIN;
    poll(pfd+i,1,300000);
    n=read(pfd[i].fd,buf,sizeof(buf));
    if(strncmp(buf,"je suis",7)==0){
      extraire(buf,8,'\n',lesplayers[i].Nom);
      Server_send(pfd[i].fd, "Tu es bien connecte.",20);
    }
  }
}

int numerotation(char *coup){
  if(strncmp(coup,"PIERRE",6)==0)
    return 0;
  else if(strncmp(coup,"FEUILLE",7)==0)
    return 1;
  else if(strncmp(coup,"CISEAUX",7)==0)
    return 2;
}

int compare(int player1, int player2){// 1 -> coup1 -1 ->coup2 0->egaliter
  
  if(player1==0 && player2==1)
    return -1;
  else if(player1==0 && player2==2)
    return 1;
  else if(player1==0 && player2==0)
    return 0;
  else if(player1==1 && player2==0)
    return 1;
  else if(player1==1 && player2==2)
    return -1;
  else if(player1==1 && player2==1)
    return 0;
  else if(player1==2 && player2==0)
    return -1;
  else if(player1==2 && player2==1)
    return 1;
  else if(player1==2 && player2==2)
    return 0;
}

void Round (struct pollfd *pfd, Player *lesplayers){//fonction aness menaa 
  int tempo, util, res, i, n;
  tempo = 20000;
  int utilitaire[3] = {0,0,0};
  char buf[1024];
  struct timeb t1, t2;
  for (i = 2 ; i > 0 ; i--){
    ftime(&t1);
    if ((n = poll(pfd + utilitaire[0], i, tempo)) < 0) {
      perror("poll");
      exit (1);
    }
    if (pfd[0].revents & POLLIN && utilitaire[1] != 1){
      read (pfd[0].fd, buf, sizeof(buf));
      lesplayers[0].coup = numerotation(buf);
      utilitaire[0] = 1;
      utilitaire[1] = 1;
    }
    if (pfd[1].revents & POLLIN && utilitaire[2] != 1){
      read (pfd[1].fd, buf, sizeof(buf));
      lesplayers[1].coup = numerotation(buf);
      utilitaire[2] = 1;
    }
    ftime(&t2);
    tempo -= (t2.time + t2.millitm) - (t1.time + t1.millitm);
  }
  
  res = compare(lesplayers[0].coup, lesplayers[1].coup);
  if (res == 1)
    lesplayers[0].score++;
  if (res == -1)
    lesplayers[1].score++;
  
  Server_sendf (pfd[0].fd, "score/coup %d-%d/%d-%s\n", 
		lesplayers[0].score, lesplayers[1].score, lesplayers[1].coup, lesplayers[1].Nom);
  Server_sendf (pfd[1].fd, "score/coup %d-%d/%d-%s\n", 
		lesplayers[1].score, lesplayers[0].score, lesplayers[0].coup, lesplayers[0].Nom);
}

int
server(int argc, char *argv[],Player *lesplayers)
{
  
  int sock;
  struct sockaddr_in addr;
  struct hostent *host;
  struct pollfd pfd[2];
  char buf[512];
  int n;
  
  if (argc < 2) {
    printf("Usage: %s [host] port\n", argv[0]);
  }
  
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }
  
  if (argc == 2) {
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atoi(argv[1]));
    
    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      perror("bind");
      exit(1);
    }
    
    if (listen(sock, 8) < 0) {
      perror("listen");
      exit(1);
    }
    
    Client_accept(pfd,sock,lesplayers);
    
    while (lesplayers[0].score < 3 && lesplayers[1].score < 3){
      Server_send(pfd[0].fd, "C'est partie.",13);
      Server_send(pfd[1].fd, "C'est partie.",13);
      Round(pfd,lesplayers);
      sleep(5);
    }
    if(lesplayers[0].score ==3){
      Server_sendf(pfd[0].fd,"Le gagnant est %s\n",lesplayers[0].Nom);
      Server_sendf(pfd[1].fd,"Le gagnant est %s\n",lesplayers[0].Nom);
    }
    else{
      Server_sendf(pfd[0].fd,"Le gagnant est %s\n",lesplayers[1].Nom);
      Server_sendf(pfd[1].fd,"Le gagnant est %s\n",lesplayers[1].Nom);
    }
  }
}


































int main(int argc, char *argv[]){
  struct pollfd* pfd;
  Player lesplayers[2];
  lesplayers[0].score = 0;
  lesplayers[1].score = 0;
  server(argc,argv,lesplayers);
  return 0;
}













