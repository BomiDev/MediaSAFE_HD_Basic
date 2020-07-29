#include <stdio.h> //ǥ������¶��̺귯��
#include <stdlib.h> //ǥ������¶��̺귯��
#include <unistd.h> //fork�����̺귯��
#include <errno.h> //�����ڵ� ��ũ�� ����
#include <string.h> //���ڿ�ó�� ���̺귯��
#include <fcntl.h> //���ϰ��� ���̺귯��
#include <signal.h> //�ñ׳�ó�� ���̺귯��
#include <sys/types.h> //�ý��۰��� ���̺귯��
#include <sys/socket.h> //��Ʈ��ũ��� ���̺귯��
#include <netinet/in.h> //���ͳ��ּ�ü�� ��� ���̺귯��
#include <arpa/inet.h> //��Ŭ�����ϻ�� ���̺귯��
#include <sys/stat.h> // �������� ���̺귯��
#define BUFSIZE 1012 // ���������� ����
#define LOG   44 //�α� ����
#define HOME /index.html //home ����

char  key_byffers[100][1000]; //key buffers
int key_cnt=0;


// BASE64
static char __base64_table[] ={
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
   'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
   'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/', '\0'
};

static char __base64_pad = '=';


struct stat s; //�Ʒ����� ���� ũ�⸦ ���ϱ� ���ؼ� ����� 
struct {//����ü 
	char *ext; //char ���� ���� ����
	char *filetype; // char ���� ���� ����
} extensions [] = {
	{"gif", "image/gif" },  //gif 
	{"jpg", "image/jpg"},    //jpg
	{"jpeg","image/jpeg"},   //jpeg
	{"png", "image/png" },  //png
	{"htm", "text/html" },  //htm
	{"html","text/html" },  //html
	{"mp4","video/mp4" },  //mp4
	{"css","text/css" },  //mp4
	{0,0} };//NULL



void log(char *s1, char *s2, int size)//�α� �Ű����� 
{
	int fpp;//�α׿� ���� ���ؼ� ���� 
	char logbuffer[200];//�α� �� �� 
	sprintf(logbuffer,"%s %s %d\n",s1, s2,size); //s0=send/req, s1= ip ,s2= path/filename , size=ũ��,num=����    
	if((fpp= open("./logs/server.log",O_WRONLY | O_APPEND,0644)) >= 0) {// ������ ����. 
		write(fpp,logbuffer,strlen(logbuffer)); //������ ������ �α׿� �ۼ��Ѵ�. 
		close(fpp);//type�� close���ش�. 
	}
}

unsigned char *__base64_decode(const unsigned char *str,int length,int *ret_length) {
   const unsigned char *current = str;
   int ch, i = 0, j = 0, k;
   /* this sucks for threaded environments */
   static short reverse_table[1000];
   static int table_built;
   unsigned char *result;

   if (++table_built == 1) {
      char *chp;
      for(ch = 0; ch < 1000; ch++) {
         chp = strchr(__base64_table, ch);
         if(chp) {
            reverse_table[ch] = chp - __base64_table;
         } else {
            reverse_table[ch] = -1;
         }
      }
   }

   result = (unsigned char *)malloc(length + 1);
   if (result == NULL) {
      return NULL;
   }

   /* run through the whole string, converting as we go */
   while ((ch = *current++) != '\0') {
      if (ch == __base64_pad) break;

      /* When Base64 gets POSTed, all pluses are interpreted as spaces.
         This line changes them back.  It's not exactly the Base64 spec,
         but it is completely compatible with it (the spec says that
         spaces are invalid).  This will also save many people considerable
         headache.  - Turadg Aleahmad <turadg@wise.berkeley.edu>
      */

      if (ch == ' ') ch = '+';

      ch = reverse_table[ch];
      if (ch < 0) continue;

      switch(i % 4) {
      case 0:
         result[j] = ch << 2;
         break;
      case 1:
         result[j++] |= ch >> 4;
         result[j] = (ch & 0x0f) << 4;
         break;
      case 2:
         result[j++] |= ch >>2;
         result[j] = (ch & 0x03) << 6;
         break;
      case 3:
         result[j++] |= ch;
         break;
      }
      i++;
   }

   k = j;
   /* mop things up if we ended on a boundary */
   if (ch == __base64_pad) {
      switch(i % 4) {
      case 0:
      case 1:
         free(result);
         return NULL;
      case 2:
         k++;
      case 3:
         result[k++] = 0;
      }
   }
   if(ret_length) {
         *ret_length = j;
   }

   result[k] = '\0';
   return result;
}

int web_run()//�����Լ� 
{

	for (int kn=0;kn<100;kn++ ){
		memset(key_byffers[kn], 0, 1000);
	}

	int web_server_end=0;
	int err_ret=0;
	int ff;//�α� ������ �缳���ϱ� ���ؼ�  
	ff=open("./logs/server.log", O_CREAT|O_TRUNC,0644);//�α������� �����ش�. 
	//printf("start\n");
	close(ff);//�α������� �ݾ��ش�. 
	int i, port,listenfd, socketfd, hit;//int������ ���� 
	pid_t pid;//��ũ�� ����ϱ� ���� ���� 
	size_t length;//
	static struct sockaddr_in cli_addr,serv_addr; //���� ����� ���� ����ü 
	
	char *path="."; // path�� ��� ����

    path = getcwd(NULL, BUFSIZ);

   
	port =9000; //�Է¹��� ��Ʈ���� port�� ���� 


	signal(SIGCLD, SIG_IGN);  // �ڽ����μ����� �ϳ��� ����Ǹ� �θ𿡰� ��ȣ���� 
	signal(SIGHUP, SIG_IGN);  // ����� �͹̳��� ���� ����
	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0){ //���� ���ϱ���� �� 
		perror("error");//���� 
		exit(1);//������. 
	}

	int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    error("setsockopt(SO_REUSEADDR) failed");

	memset((char*)&serv_addr,'\0',sizeof(serv_addr));//�ʱ�ȭ 
	serv_addr.sin_family = AF_INET;//���� �ּ� ����ü1 
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//all ip?���� �ּ� ����ü2 
	serv_addr.sin_port = htons(port);//�־��� ��Ʈ�� ���� ���� �ּ� ����ü3
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0){//���Ͽ� �̸� �����ϱ� 
		perror("error");//bind���н� ��� 
		exit(1); //������. 
	}
	
	if( listen(listenfd,500000) <0){//Ŭ���̾�Ʈ ���� ��ٸ��� 
		perror("error");//listen���н� ��� 
		exit(1);//������. 
	}

	char *buff;//������ �������� 
	for(hit=1;;hit++){//while���� ���� 
		length = sizeof(cli_addr);//cli_addr����� length�� �����Ѵ�. 
		
		printf("[web start] =================== : %d  \n",hit); 
		

		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){//�����û ���� 
			perror("error");//accept�� �߾ȵǸ� ���� 
			exit(1);//������. 
		}
		buff=inet_ntoa(cli_addr.sin_addr);//�����Ǹ� buff�� �����Ѵ�. 
		

		//
		char file_name[BUFSIZE];//���� �̸�
		// int size;//���� ũ�⸦ ���ϱ� ���� ���� ���� 
		int j, file_fd, buflen, len;//int������ ���� 
		int range_ok=1;
		int i, ret;//int�� ���� ���� 
		char * fstr;//content type�� ������ ���ڿ� ���� 
		static char buffer[BUFSIZE+1];//���� ����   
		static char xbuffer[100];//���� ����   
		ret =read(socketfd,buffer,BUFSIZE); //fd���� ��� �о��  
		printf("[info] %d =================== \n",key_cnt); 
				printf("%s\n",buffer); 
		printf("[info] =================== \n");
		char *srange;
				
		/*
			range 
		*/
		if(strstr(buffer,"Range: bytes")){ //���� ���� �̸��� &end �ִٸ�? 
			srange=strstr(buffer,"Range: bytes");
			strcpy(xbuffer,srange+13);
			for(i=0;i<100;i++) { //GET /images/05_08-over.gif �̷������� ������� 
				if(xbuffer[i] == '-') { //������ Ȯ�� 
					xbuffer[i] = 0;//�����϶� 0 
					break;//for�� Ż�� 
				}
			}
					
		}
		
		range_ok=0;
				
		if(ret == 0 || ret == -1) {//�б� �����ϸ� 
			exit(1);//������. 
		}
		if(ret > 0 && ret < BUFSIZE)  //ret�� 0���� ũ�� BUFSIZE���� ������ 
			buffer[ret]=0;   //buffer[ret]�� 0�� �ȴ�. 
		else buffer[0]=0;//���� �������� �ʴ´ٸ� buffer[0]=0�̵ȴ�. 
			for(i=4;i<BUFSIZE;i++) { //GET /images/05_08-over.gif �̷������� ������� 
				if(buffer[i] == ' ') { //������ Ȯ�� 
						buffer[i] = 0;//�����϶� 0 
						break;//for�� Ż�� 
				}
			}
				
			buflen=strlen(buffer); // buflen�� buffer���� ����
			fstr = NULL;//null�� �ʱ�ȭ 
			for(i=0;extensions[i].ext != 0;i++) { // ����ü �� Ž��
				len = strlen(extensions[i].ext); // ��������
				if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) { // ������ ���� ���������� ��
					fstr =extensions[i].filetype; //gif�����̸� image/gif�� 
					break;//for���� ������. 
				}
			}
			strcpy(file_name,&buffer[5]);//buffer[5] �� ���� �̸��� filename�� �������ش�. 
			char *rfile_name = strtok(file_name, "?");    //ù��° strtok ���.

			
		printf("[file] %s\n",rfile_name); 

		if(strstr(rfile_name,".rtsp")){ //���� ���� �̸��� &end �ִٸ�? 
			if (key_cnt>100){ key_cnt=0;}

			
			err_ret=0;
			for (int kn=0;kn<100 ;kn++ ){
				if (strlen(key_byffers[kn])==0){
					continue;
				}
				if(strstr(key_byffers[kn],rfile_name)){ 
					sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>DRM KEY ERROR</H1></BODY></HTML>\r\n");//200���� ����� ���� 
					write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
					log(buff,file_name,9);//�α��ۼ�    
					close(socketfd);//socketfd�� �ݴ´�. 
					err_ret=1;
					break;

				}
				printf("[rtsp key] [%d] %s<===\n",kn,key_byffers[kn]); 

			}
			if (err_ret==0){
				strncpy(key_byffers[key_cnt], rfile_name, strlen(rfile_name));
				key_cnt++;
			}

		}
		
		if((pid = fork()) < 0) {//�ȿ����� 
			exit(1);//������. 
		}else{//�ƴϸ� ���� 
			if(pid == 0) {////////////////////fork���� 
				close(listenfd);//listenfd�� �ݾ��ش�. 
				/*�θ𿡰� �� ����.*/
			
					if(strstr(file_name,".rtsp") && err_ret==0){ //���� ���� �̸��� &end �ִٸ�? 
						
						
						sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\n\r\n", "video/mp4");//200���� ����� ���� 
						write(socketfd,buffer,strlen(buffer));//socekfd�� ���۸� ���ش�.
						char rtsp_enc_url[BUFSIZE];
						char* rtsp64_url;
						int ret_len=0;
						memset(rtsp_enc_url, 0, BUFSIZE);
						
						strncpy(rtsp_enc_url, rfile_name, strlen(rfile_name)-5);

						rtsp64_url=__base64_decode(rtsp_enc_url,strlen(rtsp_enc_url)+1,&ret_len);
						
						seed_cbc_durl(rtsp64_url,ret_len);
						printf("[rtsp url] %s<===\n",rtsp64_url); 

						if (strlen(rtsp64_url)>0){
							rtsp_hls(rtsp64_url,socketfd);
						}
						
						close(socketfd);//socketfd�� �ݴ´�. 
						printf("[rtsp end] ===================   \n",buffer); 
						break;
					}
					
					
					sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");//200���� ����� ���� 
					write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
					log(buff,file_name,9);//�α��ۼ�    
					
					exit(1);//������. 
			
			} 
			else {
				close(socketfd);//socketfd�� �ݴ´�. 
			}
		}
		
		printf("[end loop]  ====================================== %d - end flag: %d \n",hit,web_server_end); 
	}
	printf("[end last]  ====================================== \n"); 
	return 0; // 0��ȯ
}