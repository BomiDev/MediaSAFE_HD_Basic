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
	{0,0} };//NULL



void log(char *s1, char *s2, int size)//�α� �Ű����� 
{
	int fpp;//�α׿� ���� ���ؼ� ���� 
	char logbuffer[200];//�α� �� �� 
	sprintf(logbuffer,"%s %s %d\n",s1, s2,size); //s0=send/req, s1= ip ,s2= path/filename , size=ũ��,num=����    
	if((fpp= open("./server.log",O_WRONLY | O_APPEND,0644)) >= 0) {// ������ ����. 
		write(fpp,logbuffer,strlen(logbuffer)); //������ ������ �α׿� �ۼ��Ѵ�. 
		close(fpp);//type�� close���ش�. 
	}
}

int web_run()//�����Լ� 
{
		
	int web_server_end=0;
	int ff;//�α� ������ �缳���ϱ� ���ؼ�  
	ff=open("./server.log", O_CREAT|O_TRUNC,0644);//�α������� �����ش�. 
	//printf("start\n");
	close(ff);//�α������� �ݾ��ش�. 
	int i, port,listenfd, socketfd, hit;//int������ ���� 
	pid_t pid;//��ũ�� ����ϱ� ���� ���� 
	size_t length;//
	static struct sockaddr_in cli_addr,serv_addr; //���� ����� ���� ����ü 
	
	char *path="."; // path�� ��� ����

    path = getcwd(NULL, BUFSIZ);

   
	port =8080; //�Է¹��� ��Ʈ���� port�� ���� 


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
	
	if( listen(listenfd,1000) <0){//Ŭ���̾�Ʈ ���� ��ٸ��� 
		perror("error");//listen���н� ��� 
		exit(1);//������. 
	}

	
	int fd[2];
	char buffpip[BUFSIZ];
	int state = pipe(fd);

	char *buff;//������ �������� 
	for(hit=1;;hit++){//while���� ���� 
		length = sizeof(cli_addr);//cli_addr����� length�� �����Ѵ�. 
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0){//�����û ���� 
			perror("error");//accept�� �߾ȵǸ� ���� 
			exit(1);//������. 
		}
		buff=inet_ntoa(cli_addr.sin_addr);//�����Ǹ� buff�� �����Ѵ�. 
		if((pid = fork()) < 0) {//�ȿ����� 
			exit(1);//������. 
		}
		else{//�ƴϸ� ���� 
			if(pid == 0) {////////////////////fork���� 
				close(listenfd);//listenfd�� �ݾ��ش�. 
				char file_name[50];//���� �̸�
				// int size;//���� ũ�⸦ ���ϱ� ���� ���� ���� 
				int j, file_fd, buflen, len;//int������ ���� 
				int i, ret;//int�� ���� ���� 
				char * fstr;//content type�� ������ ���ڿ� ���� 
				static char buffer[BUFSIZE+1];//���� ����   
				ret =read(socketfd,buffer,BUFSIZE); //fd���� ��� �о��  
				//log(buffer,file_name,strlen(buffer)); //�α��ۼ� 

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
				if( !strncmp(&buffer[0],"GET /\0",6))//GET /\0�϶�  
					strcpy(buffer,"GET /index.html");   //index.html����ϵ��� request���� 
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

				
				sprintf(path,"%s/%s",path,rfile_name);//path�� path/filename�� ���ش�. 

				printf("%s\n",path); 

				/*�θ𿡰� �� ����.*/
				write(fd[1], "NON", 3);
				file_fd = open(path,O_RDONLY); //get�� ����װ� ������ ��� ���� �����϶� 
				fstat(file_fd,&s);//�������� �ƴ��� Ȯ���ϱ� ���ؼ� ��� 
				if(file_fd==-1){//������ �ƴ϶��? 
					if(strstr(file_name,"key.htm")){ //���� ���� �̸��� &end �ִٸ�? 
						sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n1234567890123456");//200���� ����� ���� 
						log(buff,file_name,s.st_size); //�α��ۼ� 
						write(socketfd,buffer,strlen(buffer));//socekfd�� ���۸� ���ش�. 
						exit(1);//������. 
					}
					
					if(strstr(file_name,"&end")){ //���� ���� �̸��� &end �ִٸ�? 
						// http://127.0.0.1:8080/&end <-- ��������.
						sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>OK SERVER END</H1></BODY></HTML>\r\n");//200���� ����� ���� 
						write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
						log(buff,file_name,9);//�α��ۼ�    

						/*�θ𿡰� ���� �� ����.*/
						write(fd[1], "END", 3);

						web_server_end=1;
						printf("[end] 0 ======================================%d rnd : %d\n",pid,web_server_end); 
						exit(1);//������. 
					}else{
						// �ش� ������ ����.
						sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");//200���� ����� ���� 
						write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
						log(buff,file_name,9);//�α��ۼ�    
					}
					exit(1);//������. 
				}
				else if(S_ISDIR(s.st_mode)){
					//�����϶�. 
					sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<HTML><BODY><H1>NOT FOUND</H1></BODY></HTML>\r\n");//200���� ����� ���� 
					write(socketfd,buffer,strlen(buffer)); //������ ������ fd�� ���ش�. 
					log(buff,file_name,9);//�α��ۼ� 
					exit(1); //���� 
				}
				/*�������� ���� ���� ����*/
				sprintf(buffer,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);//200���� ����� ���� 
				log(buff,file_name,s.st_size); //�α��ۼ� 
				write(socketfd,buffer,strlen(buffer));//socekfd�� ���۸� ���ش�. 
				while ((ret = read(file_fd, buffer, BUFSIZE)) > 0 ) {//������ �д´�. 
					write(socketfd,buffer,ret);//���� ������ ���ش�. 
				}
				exit(1);//������. 
				/*�������� ���� ���� ����*/
			} 
			else {
				close(socketfd);//socketfd�� �ݴ´�. 
			}
		}
		/*�ڽĿ��� ���� �� ����.*/
		read(fd[0], buffpip, BUFSIZE);
		if(strstr(buffpip,"END")){ //�����.!!
			close(socketfd);//socketfd�� �ݴ´�. 
			break;
		}
		printf("[end loop]  ====================================== %d - end flag: %d == %s \n",hit,web_server_end,buffpip); 
	}
	printf("[end last]  ====================================== \n"); 
	return 0; // 0��ȯ
}