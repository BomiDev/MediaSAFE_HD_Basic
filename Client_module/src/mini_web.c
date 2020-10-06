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
#include <netdb.h>
#include <netdb.h>
#include "config_ctr.c"

#define BUFSIZE 1012 // ���������� ����
#define MP4_BUFSIZE 4096 // ���������� ����
#define LOG   44 //�α� ����
#define MAX_PID   100 //pid �ִ� �� ����
#define HOME /index.html //home ����

#define VER 1000 // ��������

char  key_byffers[100][1000]; //key buffers
int key_cnt=0;
pid_t pid[MAX_PID];

// BASE64 ARRAY
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

/*
	char to base64 decode 
*/
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

/*
	web_run main
*/
void err_key(int socket_id){
	static char hbuffer[BUFSIZE+1];//���� ����  
	sprintf(hbuffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>DRM KEY ERROR</H1></BODY></HTML>\r\n");
	write(socket_id,hbuffer,strlen(hbuffer)); //������ ������ fd�� ���ش�. 
	close(socket_id);//socketfd�� �ݴ´�. 
}
/*
*/


void jsonp(char* org_urls,char *des_key){

	int splitptr_cnt=0;
	static char hbuffer[BUFSIZE+1];//���� ����  
	strcpy(hbuffer,org_urls);

	char *splitptr = strtok(hbuffer, "=");  
	
	while (splitptr != NULL){
			if(strstr(splitptr,"jQuery")){ 
				char *splitptr2 = strtok(splitptr, "&");  
				while (splitptr2 != NULL){
					strcpy(des_key,splitptr2);
					break;
				}
				break;
			}
			splitptr_cnt++;
			splitptr = strtok(NULL, "="); 
	}

}

char *replaceAll(char *s, const char *olds, const char *news) {
  char *result, *sr;
  size_t i, count = 0;
  size_t oldlen = strlen(olds); if (oldlen < 1) return s;
  size_t newlen = strlen(news);


  if (newlen != oldlen) {
    for (i = 0; s[i] != '\0';) {
      if (memcmp(&s[i], olds, oldlen) == 0) count++, i += oldlen;
      else i++;
    }
  } else i = strlen(s);


  result = (char *) malloc(i + 1 + count * (newlen - oldlen));
  if (result == NULL) return NULL;


  sr = result;
  while (*s) {
    if (memcmp(s, olds, oldlen) == 0) {
      memcpy(sr, news, newlen);
      sr += newlen;
      s  += oldlen;
    } else *sr++ = *s++;
  }
  *sr = '\0';

  return result;
}


void file_dump(char *ffname,long seek,long lens,char *data ,int reset){
	long fsize=lens;


	if (reset==1){
		long freal=0;

		int32_t buf[256]; // Block size.
		memset(buf, 0, 256);

		char dump_filen[1000];
		memset(dump_filen, 0, 1000);
		sprintf(dump_filen,"/var/log/yoonagent/%s.mp4",ffname);

		FILE* file = fopen(dump_filen, "wb");
		int blocksToWrite = 1024*1024*10; // 1 GB

		for (int i = 0; i < blocksToWrite; ++i){
			 // fwrite(buf, 1, 100, file);
			 // fseek(pFile, 9, SEEK_SET);
			 // printf("%d - %d \n",fsize,freal);

			 if (fsize>=freal+1024){
				fwrite(buf, sizeof(int32_t), 256, file);
			 }else{
				int freal2=	fsize - freal;
				printf("reset ==== %d - %d \n",fsize,freal2);
				fwrite(buf, 1, freal2, file);
			   break;
			 }
			  freal = freal + 1024;
		}
		fclose(file);
	}else{
		char dump_filen[1000];
		memset(dump_filen, 0, 1000);
		sprintf(dump_filen,"/var/log/yoonagent/%s.mp4",ffname);

		FILE* file = fopen(dump_filen, "r+b");
			fseek(file, seek, SEEK_SET);
			fwrite(data, 1, lens, file);
		fclose(file);
	}
}

/*
	web_run main
*/
int web_run(){

	struct stat st = {0};
	if (stat("/var/log/yoonagent/" , &st) == -1) {
		mkdir("/var/log/yoonagent/" , 0700);
	}

	

	/* License Key init */
	for (int kn=0;kn<100;kn++ ){
		memset(key_byffers[kn], 0, 1000);
	}

	int err_ret=0;
	
	int ff;//�α� ������ �缳���ϱ� ���ؼ�  
	ff=open("./logs/server.log", O_CREAT|O_TRUNC,0644);//�α������� �����ش�. 
	close(ff);//�α������� �ݾ��ش�. 
	
	int i, port,listenfd, socketfd, hit;//int������ ���� 
	
	size_t length;//
	static struct sockaddr_in cli_addr,serv_addr; //���� ����� ���� ����ü 
	
	char *path="."; // path�� ��� ����
    path = getcwd(NULL, BUFSIZ); // ���� ���� ��� ���.

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

	for(hit=0;;hit++){//while���� ���� 
		if (hit>MAX_PID){
			hit=0;
		}
		length = sizeof(cli_addr);//cli_addr����� length�� �����Ѵ�. 

		//printf("\n\n[Socekt Start] =================== : %d  \n",hit); 
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){//�����û ���� 
			perror("error");//accept�� �߾ȵǸ� ���� 
			exit(1);//������. 
		}
		
		buff=inet_ntoa(cli_addr.sin_addr);//�����Ǹ� buff�� �����Ѵ�. 

		char file_name[BUFSIZE];//��û �̸�
		int j, buflen, len;//int������ ���� 
		int i, ret;//int�� ���� ���� 
		char * fstr;//content type�� ������ ���ڿ� ���� 
		static char buffer[BUFSIZE+1];//���� ����   
		
		static char range_start[100]={0}; //range start array ����   
		static char range_end[100]={0}; //range start array ����   
		static char http_reffer[4096]={0}; //���� ����  
		
		char *jsonp_key[4096]; //���� ���� 

		memset(range_start, 0, 100);
		memset(range_end, 0, 100);

		memset(jsonp_key, 0, 4096);
		memset(buffer, 0, BUFSIZE);
		memset(file_name, 0, BUFSIZE);
		memset(http_reffer, 0, 4096);

		ret =read(socketfd,buffer,BUFSIZE); //fd���� ��� �о��  
		//printf("\n\n[info][start] Key Cnt: %d =================== \n\n",key_cnt); 
		//		printf("%s\n",buffer); 
		//printf("[info][end] =================================\n\n");
				
		/*
			range , reffer 
		*/
		//printf("[Range][Start] =================================\n\n");
		char *srange;
		if(strstr(buffer,"Range: bytes")){ //Http Header Range
			srange=strstr(buffer,"Range: bytes");
			memcpy(range_start,srange+13,100);
			memcpy(range_end,srange+13,100);
			int end_line=0;
			for(i=0;i<100;i++) { 
				if(range_end[i] == '-') { //������ Ȯ�� 
					range_start[i] = 0;//�����϶� 0 
					memcpy(range_end,srange+14+i,100);
					end_line=i;
					if (range_end[0]=='\r'){
						range_end[0]=0;
						break;
					}
				}
				if(range_end[i] == '\r') { //������ Ȯ�� 
					if (end_line==1){
						//range_end[0]=0x30;
						range_end[0]=0;
					}else{
						range_end[i]=0;
					}
					break;
				}
			}
			//printf("[Header Range] %s-%s<=================== \n\n",range_start,range_end);
		}
		//printf("[Range][End] =================================\n\n");
		
		//printf("[REFERER][Start] =================================\n\n");
		char *referer;
		if(strstr(buffer,"Referer: ")){ //Http Header Referer
			referer=strstr(buffer,"Referer: ");
			strcpy(http_reffer,referer);
			for(i=0;i<4096;i++) { 
				if(http_reffer[i] == '\r') { //������ Ȯ�� 
					http_reffer[i]=0;
					break;
				}
			}	
		}
		//printf("[REFERER][End] =================================\n\n");
		
		if(ret == 0 || ret == -1) {//�б� �����ϸ� 
			printf("[socketfd][ret][Err] =================================\n\n");
			break;
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
			
			/* MimeType */
			fstr = NULL;//null�� �ʱ�ȭ 
			for(i=0;extensions[i].ext != 0;i++) { // ����ü �� Ž��
				len = strlen(extensions[i].ext); // ��������
				if( !strncmp(&buffer[buflen-len], extensions[i].ext, len)) { // ������ ���� ���������� ��
					fstr =extensions[i].filetype; //gif�����̸� image/gif�� 
					break;//for���� ������. 
				}
			}
			strcpy(file_name,&buffer[5]);//buffer[5] �� ���� �̸��� filename�� �������ش�. 
			
			jsonp(file_name,jsonp_key);

			char *rfile_name = strtok(file_name, "?");    //ù��° strtok ���.

			
		//printf("[param] %s\n",rfile_name); 
		//printf("[REFERER] %s\n",http_reffer); 

		/* .rtsp License Key Check */
		if( strstr(rfile_name,".rtsp")  ){
			if (key_cnt>100){ key_cnt=0;}

			err_ret=0;
			for (int kn=0;kn<100 ;kn++ ){
				if (strlen(key_byffers[kn])==0){
					continue;
				}
				if(strstr(key_byffers[kn],rfile_name)){ 
					err_key(socketfd);
					log(buff,file_name,9);//�α��ۼ�    
					err_ret=1;
					break;

				}
				printf("[drm key] [%d] %s<===\n",kn,key_byffers[kn]); 

			}
			if (err_ret==0){
				strncpy(key_byffers[key_cnt], rfile_name, strlen(rfile_name));
				key_cnt++;
			}

		}
		
		/*
			Fork Child Create..
		*/
		if((pid[hit] = fork()) < 0) {//�ȿ����� 
			printf("[err] fork Create Error\n"); 
			break;
		}else{//�ƴϸ� ���� 
			/*
				Fork Child Start  
			*/
			if(pid[hit] == 0) {
				close(listenfd);//listenfd�� �ݾ��ش�. 
					
					if(strstr(file_name,".info_mp4") && err_ret==0){ 
							sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n%s({'ret':'%s','ver':'%d'})",jsonp_key,"0",VER);//200���� ����� ���� 
							write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
							//log(buff,file_name,9);//�α��ۼ�    
							close(socketfd);//socketfd�� �ݴ´�. 
						//	printf("%s[info end] ===================   \n",buffer); 
							exit(1);//������.
					}
					/*
						http progressive download mp4 play start
					*/
					if(strstr(file_name,".mp4") && err_ret==0){ 

						printf("[mp4 Start!!] \n"); 

						char dump_file[BUFSIZE];
						memset(dump_file, 0, BUFSIZE);
						if (strlen(strstr(file_name,".mp4"))>5){
								strncpy(dump_file,strstr(file_name,".mp4")+4,strlen(strstr(file_name,".mp4"))-4);
								printf("[mp4 org]%s<===\n",dump_file); 
						}

						char mp4_enc_url[BUFSIZE];
						char* mp464_url;
						int ret_len=0;
						memset(mp4_enc_url, 0, BUFSIZE);
						
						strncpy(mp4_enc_url, rfile_name, strlen(rfile_name)-strlen(strstr(file_name,".mp4")));

						//printf("[mp4 url]%s<===\n",mp4_enc_url); 

						mp464_url=__base64_decode(mp4_enc_url,strlen(mp4_enc_url)+1,&ret_len);
						
						seed_cbc_durl(mp464_url,ret_len,http_reffer);
						if (strlen(mp464_url)<1){
							//printf("[mp4 url][%s] Null Error <===\n",mp464_url); 
							//err_key(socketfd);
							//exit(1);
						}
							
						strcpy(mp464_url,mp464_url+7);
						//printf("[mp4 url]%s<===\n",mp464_url); 

						if (strlen(mp464_url)>0){

							int   port=80;
							char  org_url[BUFSIZE]={0};
							char  host_name[BUFSIZE]={0};
							char  host_port[BUFSIZE]={0};
							char  mp4_file[BUFSIZE]={0};
							strcpy(org_url,mp464_url);

							int splitptr_cnt=0;
							char *splitptr = strtok(org_url, "/");  

							while (splitptr != NULL){
								if (splitptr_cnt==0){
									strcpy(host_name,splitptr);
									break;
								}
								splitptr_cnt++;
								splitptr = strtok(NULL, "/"); 
							}
							if (strstr(host_name,":")){
								int iii=0;
								for(iii=0;iii<100;iii++) { 
									if(host_name[iii] == ':') { 
										strcpy(host_port,host_name+iii+1);
										host_name[iii] = 0;
										break;
									}
								}
							}
							if (strlen(host_port)<1){
								strcpy(mp4_file,mp464_url+strlen(host_name)+0);
							}else{
								port=atoi(host_port);
								strcpy(mp4_file,mp464_url+strlen(host_name)+strlen(host_port)+1+0);
							}

							struct hostent *host_entry;
							host_entry = gethostbyname(host_name);
							if (!host_entry){
								printf( "host to ip Error/n");
								exit( 1);
							}
							
							//printf("[mp4      host]%s<===\n",host_name); 
							//printf("[mp4 host   ip mp4exit]%s<===\n",inet_ntoa( *(struct in_addr*)host_entry->h_addr_list[0]) ); 
							//printf("[mp4 host port]%d<===\n",port); 
							//printf("[mp4      file]%s<===\n",mp4_file); 
							
							

							struct sockaddr_in    server_addr;
							memset( &server_addr, 0, sizeof( server_addr));
							server_addr.sin_family = AF_INET;
							server_addr.sin_port = htons(port);
							server_addr.sin_addr.s_addr=  inet_addr( inet_ntoa( *(struct in_addr*)host_entry->h_addr_list[0]) );
							//printf("[mp4 Connect] Connect to %s: ", inet_ntoa(server_addr.sin_addr));

							int mp4_socket;
							mp4_socket = socket( AF_INET, SOCK_STREAM, 0);
							if( -1 == mp4_socket){
							   printf( "mp4_socket create error\n");
							   exit(1);
							}

							if( -1 == connect( mp4_socket, (struct sockaddr*)&server_addr,  sizeof(server_addr) ) )
							{
							   printf( "mp4_server connect fail\n");
							   exit(1);
							}

							 
							if (strlen(range_start)<1){
								strcpy(range_start,"0");
							}
							sprintf(buffer,
								"GET %s HTTP/1.1\r\nHost: %s\r\nConnection: keep-alive\r\nAccept: */*\r\nUser-Agent: %s\r\nRange: bytes=%s-%s\r\n\r\n", 
							mp4_file,
							host_name,
							"Yoondisk_HD",
							range_start,
							range_end	
							);
							
							
							//printf("\n[mp4 host header]\n%s===================\n",buffer);

							//mp4_server put	 
							write(mp4_socket,buffer,strlen(buffer));
							int rets=0;
							unsigned long long mp4lens=0;
							unsigned long long clens=0;
							unsigned long long rlens=0;
							int dlen=0;

							int ck_cnt=0;

							static char buffer_mp4[MP4_BUFSIZE];//���� ����   
							static char buffer_mp4_enc_one[MP4_BUFSIZE];//���� ����   
							static char buffer_mp4_enc_two[MP4_BUFSIZE*2];//���� ����   
							int enc_len=0;
							char  head_str[BUFSIZE]={0};
							memset(buffer_mp4_enc_one, 0, MP4_BUFSIZE);
							memset(buffer_mp4_enc_two, 0, MP4_BUFSIZE*2);
							while(1){
								
							
								memset(buffer_mp4, 0, MP4_BUFSIZE);
								

								int rdata_=0;

								if (dlen==50){
									//printf("[mp4 recv][]read start]==========================\n"); 
								}
								
								ret =read(mp4_socket,buffer_mp4,MP4_BUFSIZE); //fd���� ��� �о��  
								if (ret<1){break;}
								
								
								if (dlen==50){
									//printf("[mp4 recv][]read end][%d] [%d]===================\n",ret,rets); 
								}

								dlen++;
								
								
								if (rets==0){
									int head_eof=0;
									int eof_ck=0;
									char *x;
									x=strstr(buffer_mp4,"\r\n\r\n");
									int indx = x ? x - buffer_mp4 : -1;
									strncpy(head_str,buffer_mp4,indx+4);

									/*
										Send HTTP Header
									*/
									char *xmins = replaceAll(head_str, "Xdrm: 1\r\n", "Cache-control: no-cache\r\n");
									strcpy(head_str,xmins);

									xmins = replaceAll(head_str, "video/drm2\r\n", "video/mp4\r\n");
									strcpy(head_str,xmins);

									



									int splitptr_cnt=0;
									char *splitptr = strtok(head_str, "\r\n");  

									while (splitptr != NULL){

										
										if (!strstr(splitptr,"Last-Modified: ") && !strstr(splitptr,"ETag: ") &&  !strstr(splitptr,"Server: ") && !strstr(splitptr,"Connection: close") ){
											write(socketfd,splitptr,strlen(splitptr));
											write(socketfd,"\r\n",2);
										}
										if (strstr(splitptr,"Content-Length: ")){
												/*
													Content-Length: XXXXXXXX
												*/
												char *pos = NULL;

												strcpy(splitptr,splitptr+16);
												mp4lens=clens=strtoll(splitptr,&pos, 10);
												if ( atoi(range_start)==0  && strlen(dump_file)>0){
														file_dump(dump_file,0,atol(splitptr),NULL ,1);
												}
										}
											splitptr_cnt++;
											splitptr = strtok(NULL, "\r\n"); 
									}
									
									write(socketfd,"\r\n",2);


									memset(head_str, 0, BUFSIZE);
									memcpy(head_str,buffer_mp4+(indx+4),ret-(indx+4));
									memcpy(buffer_mp4,head_str,ret-(indx+4));
									ret=ret-(indx+4);

									//printf("[mp4 Content-Length] --- [%d] ----  [%d]===================\n",ret , indx); 
									//printf("[mp4 Content-Length] --- [%u] ----  [%u]===================\n",mp4lens , clens);
								

								}
								if (ret>0){


								//printf("[mp4 [%d] [%u]=============================================================\n",ret,enc_len ); 
								memcpy(buffer_mp4_enc_two+enc_len,buffer_mp4,ret);
								enc_len=enc_len+ret;
								
								if (enc_len>=4096 || ( clens<=4096 &&  enc_len==clens && clens>0)){

									int d_szie=4096;
									
									if (clens<=4096){d_szie=clens;}

									memcpy(buffer_mp4,buffer_mp4_enc_two,d_szie);
									memset(buffer_mp4_enc_one, 0, MP4_BUFSIZE);
										//memcpy(buffer_mp4_enc_one, buffer_mp4_enc_two, d_szie);
										int Seed_Len = SEED_CTR_Decrypt( pbszUserKey_ctr, pbszCounter_ctr, (BYTE*)buffer_mp4, MP4_BUFSIZE, (BYTE*)buffer_mp4_enc_one );

								

									fd_set readfds;
									struct timeval tv;
									FD_ZERO(&readfds);
									FD_SET(socketfd, &readfds);
									// �� 0.1�ʰ� ��ٸ���. 
									tv.tv_sec = 0.01;
									tv.tv_usec = 0;
									// ���� ���¸� Ȯ�� �Ѵ�. 
									int state = select(socketfd+1, &readfds,(fd_set *)0, (fd_set *)0, &tv);
									if (state==1){
										//printf("[mp4exit 001 %s [%u] [%u]=============================================================\n",range_start,rlens,mp4lens ); 
										break;
									}
									if (mp4lens<1024*1024*5){
										//usleep(1000*1);
										//printf("[mp4exit ====================5m=============\n"); 
									}
									if ( strlen(dump_file)>0 ){
									   file_dump(dump_file,atoi(range_start)+rlens, d_szie,buffer_mp4_enc_one ,0);
									}
							
									int res=write(socketfd,buffer_mp4_enc_one,d_szie);
									//file_dump(NULL,atoi(range_start)+rlens, d_szie,buffer_mp4 ,0);


									clens=clens-d_szie;
									rlens=rlens+d_szie;
									enc_len=enc_len-d_szie;

									//printf("[mp4exit ==== %d ===== %s [%d] [%d]=============================================================\n",res,range_start,rlens,mp4lens ); 
								

									if (enc_len>0){
										memmove(buffer_mp4_enc_two,buffer_mp4_enc_two+d_szie,enc_len);
									}else{
										//printf("[mp4exit enc_len = 0  ==== %d ===== %s [%d] [%d]=============================================================\n",res,range_start,rlens,mp4lens ); 
									}
								
									if (enc_len<1 || clens < 4096*3){
										//printf("[mp4exit +++ [Rstart:%s] [ret:%d] [enc_len:%d] [clens:%u] [rlens:%u] [mp4lens:%u]===========\n",range_start,ret,enc_len,clens,rlens,mp4lens ); 
										
									}
									if (clens<1  ){
										// printf("[mp4exit 002 %s [%u] [%u]=============================================================\n\n",range_start,rlens,mp4lens ); 
										//printf("[mp4exit 002 %s [%u] [%u]=============================================================\n",range_start,rlens,mp4lens ); 
										
										break;
									}
									if (enc_len==clens  ){
										/* niddle */
										// printf("[mp4exit 003] %s - [%d] [%u] \n\n\n",range_start,enc_len,clens);
										d_szie=clens;
										memset(buffer_mp4, 0, MP4_BUFSIZE);
										memcpy(buffer_mp4,buffer_mp4_enc_two,MP4_BUFSIZE);
											
											//memcpy(buffer_mp4_enc_one, buffer_mp4_enc_two, d_szie);
											int Seed_Len = SEED_CTR_Decrypt( pbszUserKey_ctr, pbszCounter_ctr, (BYTE*)buffer_mp4, MP4_BUFSIZE, (BYTE*)buffer_mp4_enc_one );
										// printf("=============================================================\n");
										// for (i=0;i<d_szie+1;i++)	{printf("%02X ",buffer_mp4_enc_one[i]);}
										// printf("=============================================================\n\n\n\n\n");

										if ( strlen(dump_file)>0 ){
										  file_dump(dump_file,atoi(range_start)+rlens, d_szie,buffer_mp4_enc_one ,0);
										}

										int res=write(socketfd,buffer_mp4_enc_one,d_szie);
										break;
									}
									
									

								}
								/*
									KISA SEED CTR BLOCK Decrypt End
								*/

								}

								
								if (dlen==50){
									//printf("[mp4 recv][]write end][%d] [%d]===================\n",ret,rets); 
								}
								rets++;
								
								if (dlen==50){
									dlen=0;
								}
								

							}
							//printf("[mp4 end]  %d - %d ===================   \n",rlens,clens); 
							close(mp4_socket);

						}
						
						close(socketfd);//socketfd�� �ݴ´�. 
						
						exit(1);//������. 
					}
					/*
						http progressive download mp4 play end
					*/



					/*
						rtsp live start
					*/
					if(strstr(file_name,".rtsp") && err_ret==0){ 
						printf("[rtsp Start!!] \n"); 
						sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nConnection: keep-alive\r\nAccept-Ranges: bytes\r\n\r\n", "video/mp4");//200���� ����� ���� 
						write(socketfd,buffer,strlen(buffer));//socekfd�� ���۸� ���ش�.
						char rtsp_enc_url[BUFSIZE];
						char* rtsp64_url;
						int ret_len=0;
						memset(rtsp_enc_url, 0, BUFSIZE);
						
						strncpy(rtsp_enc_url, rfile_name, strlen(rfile_name)-5);

						rtsp64_url=__base64_decode(rtsp_enc_url,strlen(rtsp_enc_url)+1,&ret_len);
						
						seed_cbc_durl(rtsp64_url,ret_len,http_reffer);
						//printf("[rtsp url] %s<===\n",rtsp64_url); 

						if (strlen(rtsp64_url)>0){
							//printf("[rtsp start] %s<===\n",rtsp64_url); 
							rtsp_hls(rtsp64_url,socketfd);
							//printf("[rtsp end] %s<===\n",rtsp64_url); 
						}
						
						close(socketfd);//socketfd�� �ݴ´�. 
						//printf("[rtsp end] ===================   \n",buffer); 
						exit(1);//������. 
					}
					/*
						rtsp live end
					*/
					
						
					sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");//200���� ����� ���� 
					write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
					log(buff,file_name,9);//�α��ۼ�    
					
					exit(1);//������. 
			/*
				Fork Child End  
			*/		
			} 
			else {
				close(socketfd);//socketfd�� �ݴ´�. 
			}
		}
		
		//printf("[Socket loop][%d]======================================\n",hit); 
	}
	printf("[Socket End]  ====================================== \n\n\n"); 
	return 0; // 0��ȯ
}