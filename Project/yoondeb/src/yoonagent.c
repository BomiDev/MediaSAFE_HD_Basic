#include <stdio.h> //ǥ������¶��̺귯��
#include <stdlib.h> //ǥ������¶��̺귯��
#include <unistd.h> //fork�����̺귯��
#include <errno.h> //�����ڵ� ��ũ�� ����
#include <string.h> //���ڿ�ó�� ���̺귯��
#include <signal.h> //�ñ׳�ó�� ���̺귯��
#include <pthread.h>
#include "mini_web.h"
#include "rtsp_tran.h"




// ������ �Լ�
void *t_function(void *data){
  printf("[FFmpeg] Transcoding Start Real Start !!\n"); 
  rtsp_hls("rtsp://miosoft.co.kr:1935/live/natv?tcp");
}


int main (void) { 
	pthread_t p_thread[2];
	int thr_id;
	char p1[] = "thread_1";   // 1�� ������ �̸�

	printf("[FFmpeg] Transcoding Start!!\n"); 

	thr_id = pthread_create(&p_thread[0], NULL, t_function, (void *)p1);
	
	printf("[WEB] Server Start!!\n"); 
	web_run();
	printf("[WEB] Server END!!\n"); 
	
}
