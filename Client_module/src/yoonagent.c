#include <stdio.h> //ǥ������¶��̺귯��
#include <stdlib.h> //ǥ������¶��̺귯��
#include <unistd.h> //fork�����̺귯��
#include <errno.h> //�����ڵ� ��ũ�� ����
#include <string.h> //���ڿ�ó�� ���̺귯��
#include <signal.h> //�ñ׳�ó�� ���̺귯��
#include <pthread.h>
#include "mini_web.h"
#include "rtsp_tran.h"
#include "KISA_SEED_CBC.h"



// ������ �Լ�

int main (void) { 
	
	/*
	pthread_t p_thread[2];
	int thr_id;
	char p1[] = "thread_1";   // 1�� ������ �̸�

	printf("[FFmpeg] Transcoding Start!!\n"); 

	thr_id = pthread_create(&p_thread[0], NULL, t_function, (void *)p1);
	*/

	printf("[SEED] Start!!\n"); 
	//seed_test_ctr();
	printf("[SEED] End!!\n"); 

	printf("[WEB] Server Start!!\n"); 
	web_run();
	printf("[WEB] Server END!!\n"); 
	
}
