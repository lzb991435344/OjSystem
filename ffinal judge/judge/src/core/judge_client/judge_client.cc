
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <assert.h>
#include "okcalls.h"

#define IGNORE_ESOL
#define STD_MB 1048576LL
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)  //文件大小限制为32M
#define STD_M_LIM (STD_MB<<7)  //内存限制为128M
#define BUFFER_SIZE 5120       //缓冲区大小为 5120 bytes

#define OJ_WT0 0
#define OJ_WT1 1
#define OJ_CI 2
#define OJ_RI 3
#define OJ_AC 4
#define OJ_PE 5
#define OJ_WA 6
#define OJ_TL 7
#define OJ_ML 8
#define OJ_OL 9
#define OJ_RE 10
#define OJ_CE 11
#define OJ_CO 12
#define OJ_TR 13

#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi

#endif

static int DEBUG = 0;
static char host_name[BUFFER_SIZE];
static char user_name[BUFFER_SIZE];
static char password[BUFFER_SIZE];
static char db_name[BUFFER_SIZE];
static char oj_home[BUFFER_SIZE];
static char data_list[BUFFER_SIZE][BUFFER_SIZE];

//文件的总长度，和缓冲区的大小作比较
//超出缓冲区大小直接返回
static int data_list_len = 0;

static int port_number;
static int max_running;
static int sleep_time;
static int java_time_bonus = 5;
static int java_memory_bonus = 512;
static char java_xms[BUFFER_SIZE];
static char java_xmx[BUFFER_SIZE];
static int sim_enable = 0;
static int oi_mode = 0;
static int full_diff = 0;
static int use_max_time = 0;

static int http_judge = 0;
static char http_baseurl[BUFFER_SIZE];
static char http_username[BUFFER_SIZE];
static char http_password[BUFFER_SIZE];
static int http_download = 1;
static double cpu_compensation=1.0;

static int shm_run = 0;

static char record_call = 0;
static int use_ptrace = 1;
static int compile_chroot=1;
static int turbo_mode=0;

//数据库的名字
static const char* tbname="solution";

#define ZOJ_COM

#ifdef _mysql_h
MYSQL *conn;//mysql数据库连接对象
#endif

//可支持的语言列表
static char lang_ext[18][8] = { "c", "cc", "pas", "java", "rb", "sh", "py",
		"php", "pl", "cs", "m", "bas", "scm","c","cc","lua","js","go" };

int data_list_has(char * file)
{
   for(int i = 0; i < data_list_len; i++){
	   //比较两个文件的地址，相同返回1
       if(strcmp(data_list[i],file) == 0)
		return 1;
   }
   return 0;
}

int data_list_add(char * file){
	//判断文件列表的总长度
   if(data_list_len < BUFFER_SIZE - 1){
	   //复制当前文件的地址到data_list中，复制指向文件的指针变量
	  strcpy(data_list[data_list_len], file);
	  data_list_len++;//列表长度加1 
   	  return 0;
   }else{

	return 1;

   }
}

//获取文件大小
long get_file_size(const char * filename) {
	struct stat f_stat;
     //文件状态赋值到结构体中
	//获取指定路径的文件夹或文件的信息
	//成功返回0，失败返回-1
	if (stat(filename, &f_stat) == -1) {
		return 0;
	}
	//返回文件的大小，转换一下数据格式
	return (long) f_stat.st_size;
}

//写入日志文件
void write_log(const char *_fmt, ...) {
	va_list ap;
	char fmt[4096];
	strncpy(fmt, _fmt, 4096);
	char buffer[4096];
	sprintf(buffer, "%s/log/client.log", oj_home);
	FILE *fp = fopen(buffer, "ae+");
	if (fp == NULL) {
		fprintf(stderr, "openfile error!\n");
		system("pwd");
	}
	va_start(ap, _fmt);

	//根据fmt转换格式化数据，复制到buffer中
	vsprintf(buffer, fmt, ap);
	fprintf(fp, "%s\n", buffer);
	if (DEBUG)
		printf("%s\n", buffer);
	va_end(ap);
	fclose(fp);
}

int execute_cmd(const char * fmt, ...) {
	//新建一个字符数组保存
	char cmd[BUFFER_SIZE];

	int ret = 0;

	/**  va_list是C语言中用于解决变参的一组宏
    （1）首先在函数里定义一具VA_LIST型的变量，这个变量是指向参数的指针；
    （2）然后用VA_START宏初始化刚定义的VA_LIST变量；
    （3）然后用VA_ARG返回可变的参数，VA_ARG的第二个参数是你要返回的参数的
     类型（如果函数有多个可变参数的，依次调用VA_ARG获取各个参数）；
    （4）最后用VA_END宏结束可变参数的获取。
	*/
	va_list ap;

	va_start(ap, fmt);//初始化

	/**
	函数名: vsprintf
   返回值: 正常情况下返回生成字串的长度(除去\0),错误情况返回负值
   用 法: int vsprintf(char *string, char *format, va_list param);
   //将param 按格式format写入字符串string中
   注: 该函数会出现内存溢出情况,建议使用vsnprintf
	*/
	vsprintf(cmd, fmt, ap);
	printf("%s\n",cmd);

	/**
	
     function: int system(const char * string); 
    system()会调用fork()产生子进程，由子进程来调用/bin/sh-c string来执行参数string字符串所代表的命令，
    此命>令执行完后随即返回原调用的进程。在调用system()期间SIGCHLD 信号会被暂时搁置，SIGINT和SIGQUIT 
    信号则会被忽略。 
    返回值 =-1:出现错误 
    =0:调用成功但是没有出现子进程 
    >0:成功退出的子进程的id 如果
    system()在调用/bin/sh时失败则返回127，其他失败原因返回-1。
	*/
	ret = system(cmd);
	va_end(ap);//结束可变参数的获取

	return ret;
}
//定义一些全局变量
const int call_array_size = 512;
unsigned int call_id = 0;

unsigned int call_counter[call_array_size] = { 0 };
static char LANG_NAME[BUFFER_SIZE];//语言包

//该函数用于设置一些空间时间的限制
//保存在一个call_counter的无符号整型数组中
void init_syscalls_limits(int lang) {
	int i;

	//初始化无符号整型数组
	memset(call_counter, 0, sizeof(call_counter));

	if (DEBUG)
		write_log("init_call_counter:%d", lang);
	//record_call标志着是否是debug,0代表debug,
	if (record_call) { // recording for debuging
		for (i = 0; i < call_array_size; i++) {
			call_counter[i] = 0;
		}
	} else if (lang <= 1||lang==13||lang==14) { // C & C++
		for (i = 0; i==0||LANG_CV[i]; i++) {
			call_counter[LANG_CV[i]] = HOJ_MAX_LIMIT;
		}
	} else if (lang == 2) { // Pascal
		for (i = 0; i==0||LANG_PV[i]; i++)
			call_counter[LANG_PV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 3) { // Java
		for (i = 0; i==0||LANG_JV[i]; i++)
			call_counter[LANG_JV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 4) { // Ruby
		for (i = 0; i==0||LANG_RV[i]; i++)
			call_counter[LANG_RV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 5) { // Bash
		for (i = 0; i==0||LANG_BV[i]; i++)
			call_counter[LANG_BV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 6) { // Python
		for (i = 0; i==0||LANG_YV[i]; i++)
			call_counter[LANG_YV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 7) { // php
		for (i = 0; i==0||LANG_PHV[i]; i++)
			call_counter[LANG_PHV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 8) { // perl
		for (i = 0; i==0||LANG_PLV[i]; i++)
			call_counter[LANG_PLV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 9) { // mono c#
		for (i = 0; i==0||LANG_CSV[i]; i++)
			call_counter[LANG_CSV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 10) { //objective c
		for (i = 0; i==0||LANG_OV[i]; i++)
			call_counter[LANG_OV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 11) { //free basic
		for (i = 0; i==0||LANG_BASICV[i]; i++)
			call_counter[LANG_BASICV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 12) { //scheme guile
		for (i = 0; i==0||LANG_SV[i]; i++)
			call_counter[LANG_SV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 15) { //lua
		for (i = 0; i==0||LANG_LUAV[i]; i++)
			call_counter[LANG_LUAV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 16) { //nodejs
		for (i = 0; i==0||LANG_JSV[i]; i++)
			call_counter[LANG_JSV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 17) { //go
		for (i = 0; i==0||LANG_GOV[i]; i++)
			call_counter[LANG_GOV[i]] = HOJ_MAX_LIMIT;
	}
}

//去除'='号
int after_equal(char * c) {
	int i = 0;
	for (; c[i] != '\0' && c[i] != '='; i++)
		;
	return ++i;
}
//返回去除空格的字符串副本
void trim(char * c) {
	//字符数组
	char buf[BUFFER_SIZE];
	char * start, *end;

	//传进来的数组先复制到buf数组中
	strcpy(buf, c);
	start = buf;
	while (isspace(*start))
		start++;
	end = start;
	//指针值不是空格的时候，end加1
	while (!isspace(*end))
		end++;
	//添加结束符
	*end = '\0';

	//将去除了空格的字符赋值给原来的字符数组
	strcpy(c, start);
}
//读取缓存
bool read_buf(char * buf, const char * key, char * value) {
	if (strncmp(buf, key, strlen(key)) == 0) {
		strcpy(value, buf + after_equal(buf));
		trim(value);
		if (DEBUG)
			printf("%s\n", value);
		return 1;
	}
	return 0;
}
void read_double(char * buf, const char * key,double * value) {
	char buf2[BUFFER_SIZE];
	if (read_buf(buf, key, buf2))
		sscanf(buf2, "%lf", value);
}

void read_int(char * buf, const char * key, int * value) {
	char buf2[BUFFER_SIZE];
	if (read_buf(buf, key, buf2))
		sscanf(buf2, "%d", value);
}

//读取cmd的输出，返回一个指向该文件的指针
FILE * read_cmd_output(const char * fmt, ...) {
	char cmd[BUFFER_SIZE];

	FILE * ret = NULL;

	//变参处理
	va_list ap;

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);



	if (DEBUG)
		printf("%s\n", cmd);
	/**


popen（建立管道I/O）
相关函数  pipe，mkfifo，pclose，fork，system，fopen
表头文件  #include<stdio.h>

定义函数  FILE * popen( const char * command,const char * type);

函数说明  popen()会调用fork()产生子进程，然后从子进程中调用/bin/sh -c
来执行参数command的指令。参数type可使用“r”代表读取，“w”代表写入。
依照此type值，popen()会建立管道连到子进程的标准输出设备或标准输入设备，
然后返回一个文件指针。随后进程便可利用此文件指针来读取子进程的输出设备
或是写入到子进程的标准输入设备中。此外，所有使用文件指针(FILE*)操作的
函数也都可以使用，除了fclose()以外。

返回值  若成功则返回文件指针，否则返回NULL，错误原因存于errno中。
错误代码  EINVAL参数type不合法。
	
	*/
	ret = popen(cmd, "r");

	return ret;
}

// 读取配置文件
void init_mysql_conf() {
	FILE *fp = NULL;
	char buf[BUFFER_SIZE];
	host_name[0] = 0;
	user_name[0] = 0;
	password[0] = 0;
	db_name[0] = 0;
	port_number = 3306;
	max_running = 3;
	sleep_time = 3;
	/*1、-Xms ：表示java虚拟机堆区内存初始内存分配的大小，
	 通常为操作系统可用内存的1/64大小即可，但仍需按照实际情况进行分配。
	有可能真的按照这样的一个规则分配时，设计出的软件还没有能够运行得起来就挂了。
	2、-Xmx： 表示java虚拟机堆区内存可被分配的最大上限，通常为操作系统可用内存的1/4大小。
	但是开发过程中，通常会将 -Xms 与 -Xmx两个参数的配置相同的值，其目的是为了能够在java垃圾回收机制清理完堆区后不需要重新分隔计算堆区的大小而浪费资源
	*/
	strcpy(java_xms, "-Xms32m");//jvm初始分配的大小
	strcpy(java_xmx, "-Xmx256m");
	sprintf(buf, "%s/etc/judge.conf", oj_home);
	fp = fopen("./etc/judge.conf", "re");
	if (fp != NULL) {
		//循环从文件描述符中读取字符串，
		while (fgets(buf, BUFFER_SIZE - 1, fp)) {
			read_buf(buf, "OJ_HOST_NAME", host_name);
			read_buf(buf, "OJ_USER_NAME", user_name);
			read_buf(buf, "OJ_PASSWORD", password);
			read_buf(buf, "OJ_DB_NAME", db_name);
			read_int(buf, "OJ_PORT_NUMBER", &port_number);
			read_int(buf, "OJ_JAVA_TIME_BONUS", &java_time_bonus);
			read_int(buf, "OJ_JAVA_MEMORY_BONUS", &java_memory_bonus);
			read_int(buf, "OJ_SIM_ENABLE", &sim_enable);
			read_buf(buf, "OJ_JAVA_XMS", java_xms);
			read_buf(buf, "OJ_JAVA_XMX", java_xmx);
			read_int(buf, "OJ_HTTP_JUDGE", &http_judge);
			read_buf(buf, "OJ_HTTP_BASEURL", http_baseurl);
			read_buf(buf, "OJ_HTTP_USERNAME", http_username);
			read_buf(buf, "OJ_HTTP_PASSWORD", http_password);
			read_int(buf, "OJ_HTTP_DOWNLOAD", &http_download);
			read_int(buf, "OJ_OI_MODE", &oi_mode);
			read_int(buf, "OJ_FULL_DIFF", &full_diff);
			read_int(buf, "OJ_SHM_RUN", &shm_run);
			read_int(buf, "OJ_USE_MAX_TIME", &use_max_time);
			read_int(buf, "OJ_USE_PTRACE", &use_ptrace);
			read_int(buf, "OJ_COMPILE_CHROOT", &compile_chroot);
			read_int(buf, "OJ_TURBO_MODE", &turbo_mode);
			read_double(buf, "OJ_CPU_COMPENSATION", &cpu_compensation);

		}
	}
 	if(strcmp(http_username, "IP") == 0){
                  FILE * fjobs = read_cmd_output("ifconfig|grep 'inet'|awk -F: '{printf $2}'|awk  '{printf $1}'");
                  fscanf(fjobs, "%s", http_username);
                  pclose(fjobs);
        }
	if(strcmp(http_username, "HOSTNAME") == 0){
                  strcpy(http_username, getenv("HOSTNAME"));
        }
	if(turbo_mode == 2) tbname="solution2";
}

///home/judge/data/1000下的文件全名 sample.in sample.out test.in test.out
int isInFile(const char fname[]) {
	int l = strlen(fname);
	//长度小于3且不是.in结尾的程序，退出
	if (l <= 3 || strcmp(fname + l - 3, ".in") != 0)
		return 0;
	else
	 //返回文件名长度
		return l - 3;
}
//去掉文件中的空格信息，读取字符串进行比对
void find_next_nonspace(int & c1, int & c2, FILE *& f1, FILE *& f2, int & ret) {
	while ((isspace(c1)) || (isspace(c2))) {
		if (c1 != c2) {
			if (c2 == EOF) {
				do {
					c1 = fgetc(f1);
				} while (isspace(c1));
				continue;
			} else if (c1 == EOF) {
				do {
					c2 = fgetc(f2);
				} while (isspace(c2));
				continue;
#ifdef IGNORE_ESOL
			} else if (isspace(c1) && isspace(c2)) {
                                  while(c2=='\n'&&isspace(c1)&&c1!='\n') c1 = fgetc(f1);
                                  while(c1=='\n'&&isspace(c2)&&c2!='\n') c2 = fgetc(f2);
#else
           //c1为回车，c2为换行
			} else if ((c1 == '\r' && c2 == '\n')) {
				c1 = fgetc(f1);
			} else if ((c2 == '\r' && c1 == '\n')) {
				c2 = fgetc(f2);			
#endif      //c1 c2中的字符串都相等情况下
			} else {
				if (DEBUG)
					printf("%d=%c\t%d=%c", c1, c1, c2, c2);
				;
				//格式检测不正确
				ret = OJ_PE;
			}
		}
		if (isspace(c1)) {
			c1 = fgetc(f1);
		}
		if (isspace(c2)) {
			c2 = fgetc(f2);
		}
	}
}
//从路径中获取文件名字
const char * getFileNameFromPath(const char * path) {
	//长度递减，从尾部开始
	for (int i = strlen(path); i >= 0; i--) {
		if (path[i] == '/')
			return &path[i+1];
	}
	return path;
}

void make_diff_out_full(FILE * f1, FILE * f2, int c1, int c2, const char*  path) {
	
	execute_cmd("echo '========[%s]========='>>diff.out",getFileNameFromPath(path));
	execute_cmd("echo '------test in top 100 lines------'>>diff.out");
	execute_cmd("head -100 data.in>>diff.out");
	execute_cmd("echo '------test out top 100 lines-----'>>diff.out");
	execute_cmd("head -100 '%s'>>diff.out",path);
	execute_cmd("echo '------user out top 100 lines-----'>>diff.out");
	execute_cmd("head -100 user.out>>diff.out");
	execute_cmd("echo '------diff out 200 lines-----'>>diff.out");
	execute_cmd("diff '%s' user.out -y|head -200>>diff.out",path);
	execute_cmd("echo '=============================='>>diff.out");

}
void make_diff_out_simple(FILE *f1, FILE *f2, int c1, int c2, const char * path) {
	execute_cmd("echo '========[%s]========='>>diff.out",getFileNameFromPath(path));
	execute_cmd("echo 'Expected						      |	Yours'>>diff.out");
	execute_cmd("diff '%s' user.out -y|head -100>>diff.out",path);
	execute_cmd("echo '\n=============================='>>diff.out");
}


//程序运行结果和正确文件进行比较
int compare_zoj(const char *file1, const char *file2) {
  //结果默认为AC
	int ret = OJ_AC;
	int c1, c2;
	FILE * f1, *f2;
	//FILE * fopen(const char * path,const char * mode);
	//路径和文件的打开方式
	f1 = fopen(file1, "re");
	f2 = fopen(file2, "re");
	if (!f1 || !f2) {
	    //其中一个文件打不开都是运行错误，直接返回给宏
		ret = OJ_RE;
	} else
		for (;;) {
			// fgetc函数：从流文件中读取一个字符
			c1 = fgetc(f1);
			c2 = fgetc(f2);

			//去除字符串中的空白
			find_next_nonspace(c1, c2, f1, f2, ret);
			for (;;) {
				// 一直读直到两个文件都遇到空格或者都返回0
				//while循环进行读取文件
				while ((!isspace(c1) && c1) || (!isspace(c2) && c2)) {
					if (c1 == EOF && c2 == EOF) {
						goto end;
					}
					//两个文件中有一个文件走到了结尾
					if (c1 == EOF || c2 == EOF) {
						break;//直接退出循环
					}
					if (c1 != c2) {
						//两个字符串比对不相等
						ret = OJ_WA;//结果是错误答案
						goto end;
					}
					//继续从文件流中接受字符
					c1 = fgetc(f1);
					c2 = fgetc(f2);
				}
				//去除文件中的空格字符串
				find_next_nonspace(c1, c2, f1, f2, ret);
				if (c1 == EOF && c2 == EOF) {
					goto end;
				}
				//两个为空文件
				if (c1 == EOF || c2 == EOF) {
					ret = OJ_WA;
					goto end;
				}
                   //其中一个文件读到换行或者读完
				if ((c1 == '\n' || !c1) && (c2 == '\n' || !c2)) {
					break;
				}
			}
		}
	end: 
	if (ret == OJ_WA || ret==OJ_PE){
		if(full_diff)
			make_diff_out_full(f1, f2, c1, c2, file1);
		else
			make_diff_out_simple(f1, f2, c1, c2, file1);
	}
	//关闭流
	if (f1)
		fclose(f1);
	if (f2)
		fclose(f2);
	return ret;
}

void delnextline(char s[]) {
	int L;
	L = strlen(s);
	//去掉空格，换行
	while (L > 0 && (s[L - 1] == '\n' || s[L - 1] == '\r'))
		s[--L] = 0;
}
//将用户的输出文件和测试文件作对比，将结果返回给宏
int compare(const char *file1, const char *file2) {
#ifdef ZOJ_COM
	return compare_zoj(file1, file2);
#endif
#ifndef ZOJ_COM
	FILE *f1,*f2;
	char *s1,*s2,*p1,*p2;
	int PEflg;
	s1=new char[STD_F_LIM+512];
	s2=new char[STD_F_LIM+512];

	if (!(f1=fopen(file1,"re")))
	return OJ_AC;

	for (p1=s1;EOF!=fscanf(f1,"%s",p1);)
	while (*p1) p1++;
	fclose(f1);

	if (!(f2=fopen(file2,"re")))
	return OJ_RE;

	for (p2=s2;EOF!=fscanf(f2,"%s",p2);)
	while (*p2) p2++;
	fclose(f2);

	//两个文件中的字符串不匹配
	if (strcmp(s1,s2)!=0) {
		delete[] s1;
		delete[] s2;
		//答案错误
		return OJ_WA;
	} else {//字符匹配
		f1=fopen(file1,"re");
		f2=fopen(file2,"re");
		PEflg=0;
		/**
		#define STD_MB 1048576LL
		#define STD_F_LIM (STD_MB<<5)  //文件大小限制为32M
        #define STD_M_LIM (STD_MB<<7)  //内存限制为128M
		*/

		while (PEflg==0 && fgets(s1,STD_F_LIM,f1) && fgets(s2,STD_F_LIM,f2)) {
		   //先去掉空格，换行
			delnextline(s1);
			delnextline(s2);
			//两个字符串相等
			if (strcmp(s1,s2)==0) continue;
			else PEflg=1;
		}
		//s1,s2数组是new出来的，需要delete掉
		delete [] s1;
		delete [] s2;
		fclose(f1);
		fclose(f2);
		if (PEflg) return OJ_PE;
		else return OJ_AC;
	}
#endif
}
//检查用户是否已经登录
bool check_login() {
	const char * cmd =
			" wget --post-data=\"checklogin=1\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	int ret = 0;
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	fscanf(fjobs, "%d", &ret);
	pclose(fjobs);
	return ret;
}
//用户登录
void login() {
	if (!check_login()) {
		const char * cmd =
				"wget --post-data=\"user_id=%s&password=%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/login.php\"";
		FILE * fjobs = read_cmd_output(cmd, http_username, http_password,
				http_baseurl);
		pclose(fjobs);
	}

}
#ifdef _mysql_h
//core中更新到数据库
void _update_solution_mysql(int solution_id, int result, int time, int memory,
		int sim, int sim_s_id, double pass_rate) {
	char sql[BUFFER_SIZE];
	if (oi_mode) {
	//普通评测模式下的更新，会计算pass_rate值
		sprintf(sql,
				"UPDATE %s SET result=%d,time=%d,memory=%d,pass_rate=%f,judger='%s',judgetime=now() WHERE solution_id=%d LIMIT 1%c",
					tbname,	    result, time,   memory,   pass_rate,  http_username, solution_id, 0);
	} else {
	  //比赛模式下的更新
		sprintf(sql,
				"UPDATE %s SET result=%d,time=%d,memory=%d,judger='%s',judgetime=now() WHERE solution_id=%d LIMIT 1%c",
					tbname,     result, time, memory,http_username, solution_id, 0);
	}
	if (mysql_real_query(conn, sql, strlen(sql))) {
	}
	if (sim) {
		sprintf(sql,
				"insert into sim(s_id,sim_s_id,sim) values(%d,%d,%d) on duplicate key update  sim_s_id=%d,sim=%d",
				solution_id, sim_s_id, sim, sim_s_id, sim);
		if (mysql_real_query(conn, sql, strlen(sql))) {
		}
	}
}
#endif
//从web端更新数据到solution表，用户提交代码
void _update_solution_http(int solution_id, int result, int time, int memory,
		int sim, int sim_s_id, double pass_rate) {
	const char * cmd =
			" wget --post-data=\"update_solution=1&sid=%d&result=%d&time=%d&memory=%d&sim=%d&simid=%d&pass_rate=%f\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, solution_id, result, time, memory, sim,
			sim_s_id, pass_rate, http_baseurl);
	pclose(fjobs);
}
//根据参数更新solution表
void update_solution(int solution_id, int result, int time, int memory, int sim,
		int sim_s_id, double pass_rate) {
	if (result == OJ_TL && memory == 0)
		result = OJ_ML;
	if (http_judge) {
		_update_solution_http(solution_id, result, time, memory, sim, sim_s_id,
				pass_rate);
	} else {

#ifdef _mysql_h
		_update_solution_mysql(solution_id, result, time, memory, sim, sim_s_id,
				pass_rate);
#endif
	}
}
//编译信息写回数据库
#ifdef _mysql_h
void _addceinfo_mysql(int solution_id) {
	char sql[(1 << 16)], *end;
	char ceinfo[(1 << 16)], *cend;
	FILE *fp = fopen("ce.txt", "re");
	snprintf(sql, (1 << 16) - 1, "DELETE FROM compileinfo WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	cend = ceinfo;
	while (fgets(cend, 1024, fp)) {
		cend += strlen(cend);
		if (cend - ceinfo > 40000)
			break;
	}
	cend = 0;
	end = sql;
	strcpy(end, "INSERT INTO compileinfo VALUES(");
	end += strlen(sql);
	*end++ = '\'';
	end += sprintf(end, "%d", solution_id);
	*end++ = '\'';
	*end++ = ',';
	*end++ = '\'';
	end += mysql_real_escape_string(conn, end, ceinfo, strlen(ceinfo));
	*end++ = '\'';
	*end++ = ')';
	*end = 0;
	if (mysql_real_query(conn, sql, end - sql))
		printf("%s\n", mysql_error(conn));
	fclose(fp);
}
#endif
//把16进制转换成整数值
char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}
//整数值转换成16进制
char to_hex(char code) {
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

//返回字符串的编码
char *url_encode(char *str) {
	char *pstr = str, *buf = (char *) malloc(strlen(str) * 3 + 1), *pbuf = buf;
	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.'
				|| *pstr == '~')
			*pbuf++ = *pstr;
		else if (*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(
					*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}
//通过http方式写回数据库
void _addceinfo_http(int solution_id) {

	char ceinfo[(1 << 16)], *cend;
	char * ceinfo_encode;
	FILE *fp = fopen("ce.txt", "re");

	cend = ceinfo;
	while (fgets(cend, 1024, fp)) {
		cend += strlen(cend);
		if (cend - ceinfo > 40000)
			break;
	}
	fclose(fp);
	ceinfo_encode = url_encode(ceinfo);
	FILE * ce = fopen("ce.post", "we");
	fprintf(ce, "addceinfo=1&sid=%d&ceinfo=%s", solution_id, ceinfo_encode);
	fclose(ce);
	free(ceinfo_encode);

	const char * cmd =
			" wget --post-file=\"ce.post\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	pclose(fjobs);
}

void addceinfo(int solution_id) {
	if (http_judge) {
		_addceinfo_http(solution_id);
	} else {

#ifdef _mysql_h
		_addceinfo_mysql(solution_id);
#endif
	}
}

//将运行错误信息写回数据库
#ifdef _mysql_h
void _addreinfo_mysql(int solution_id, const char * filename) {
	char sql[(1 << 16)], *end;
	char reinfo[(1 << 16)], *rend;
	FILE *fp = fopen(filename, "re");
	snprintf(sql, (1 << 16) - 1, "DELETE FROM runtimeinfo WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	rend = reinfo;
	while (fgets(rend, 1024, fp)) {
		rend += strlen(rend);
		if (rend - reinfo > 40000)
			break;
	}
	rend = 0;
	end = sql;
	strcpy(end, "INSERT INTO runtimeinfo VALUES(");
	end += strlen(sql);
	*end++ = '\'';
	end += sprintf(end, "%d", solution_id);
	*end++ = '\'';
	*end++ = ',';
	*end++ = '\'';
	end += mysql_real_escape_string(conn, end, reinfo, strlen(reinfo));
	*end++ = '\'';
	*end++ = ')';
	*end = 0;
	if (mysql_real_query(conn, sql, end - sql))
		printf("%s\n", mysql_error(conn));
	fclose(fp);
}
#endif

void _addreinfo_http(int solution_id, const char * filename) {
	char reinfo[(1 << 16)], *rend;
	char * reinfo_encode;
	FILE *fp = fopen(filename, "re");

	rend = reinfo;
	//读取字符流读取数据到数组
	while (fgets(rend, 1024, fp)) {
		rend += strlen(rend);
		if (rend - reinfo > 40000)
			break;
	}
	fclose(fp);
	reinfo_encode = url_encode(reinfo);
	FILE * re = fopen("re.post", "we");
	fprintf(re, "addreinfo=1&sid=%d&reinfo=%s", solution_id, reinfo_encode);
	fclose(re);
	free(reinfo_encode);

	const char * cmd =
			" wget --post-file=\"re.post\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	pclose(fjobs);

}
void addreinfo(int solution_id) {
	if (http_judge) {
		_addreinfo_http(solution_id, "error.out");
	} else {
#ifdef _mysql_h
		_addreinfo_mysql(solution_id, "error.out");
#endif
	}
}

void adddiffinfo(int solution_id) {
	if (http_judge) {
		_addreinfo_http(solution_id, "diff.out");
	} else {
#ifdef _mysql_h
		_addreinfo_mysql(solution_id, "diff.out");
#endif
	}
}
void addcustomout(int solution_id) {

	if (http_judge) {
		_addreinfo_http(solution_id, "user.out");
	} else {
#ifdef _mysql_h
		_addreinfo_mysql(solution_id, "user.out");
#endif
	}
}
#ifdef _mysql_h

void _update_user_mysql(char * user_id) {
	char sql[BUFFER_SIZE];
	sprintf(sql,
			"UPDATE `users` SET `solved`=(SELECT count(DISTINCT `problem_id`) FROM `solution` WHERE `user_id`=\'%s\' AND `result`=\'4\') WHERE `user_id`=\'%s\'",
			user_id, user_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
	sprintf(sql,
			"UPDATE `users` SET `submit`=(SELECT count(*) FROM `solution` WHERE `user_id`=\'%s\' and problem_id>0) WHERE `user_id`=\'%s\'",
			user_id, user_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
}
#endif
void _update_user_http(char * user_id) {

	const char * cmd =
			" wget --post-data=\"updateuser=1&user_id=%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, user_id, http_baseurl);
	pclose(fjobs);
}
void update_user(char * user_id) {
	if (http_judge) {
		_update_user_http(user_id);
	} else {

#ifdef _mysql_h
		_update_user_mysql(user_id);
#endif
	}
}

void _update_problem_http(int pid) {
	const char * cmd =
			" wget --post-data=\"updateproblem=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, pid, http_baseurl);
	pclose(fjobs);
}

#ifdef _mysql_h
void _update_problem_mysql(int p_id) {
	char sql[BUFFER_SIZE];

	sprintf(sql,
			"UPDATE `problem` SET `accepted`=(SELECT count(*) FROM `solution` WHERE `problem_id`=\'%d\' AND `result`=\'4\') WHERE `problem_id`=\'%d\'",
			p_id, p_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));


	sprintf(sql,
			"UPDATE `problem` SET `submit`=(SELECT count(*) FROM `solution` WHERE `problem_id`=\'%d\') WHERE `problem_id`=\'%d\'",
			p_id, p_id);

	//返回值为0
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
}
#endif
void update_problem(int pid) {
	if (http_judge) {
		_update_problem_http(pid);
	} else {
#ifdef _mysql_h
		_update_problem_mysql(pid);
#endif
	}
}

//卸载当前的文件系统
void umount(char * work_dir){
        execute_cmd("/bin/umount -f %s/proc", work_dir);
        execute_cmd("/bin/umount -f %s/dev ", work_dir);
        execute_cmd("/bin/umount -f %s/lib ", work_dir);
        execute_cmd("/bin/umount -f %s/lib64 ", work_dir);
        execute_cmd("/bin/umount -f %s/etc/alternatives ", work_dir);
        execute_cmd("/bin/umount -f %s/usr ", work_dir);
        execute_cmd("/bin/umount -f %s/bin ", work_dir);
        execute_cmd("/bin/umount -f %s/proc ", work_dir);
        execute_cmd("/bin/umount -f bin usr lib lib64 etc/alternatives proc dev ");
        execute_cmd("/bin/umount -f %s/* ",work_dir);
	    execute_cmd("/bin/umount -f %s/log/* ",work_dir);
     	execute_cmd("/bin/umount -f %s/log/etc/alternatives ", work_dir);
}
//编译程序
int compile(int lang,char * work_dir)  {
	int pid;
     //设置基本的编译命令，指针数组存
	const char * CP_C[] = { "gcc","Main.c", "-o", "Main","-O2", "-Wall",
			"-lm", "--static","-std=c99", "-DONLINE_JUDGE", NULL };
	const char * CP_X[] = { "g++", "-fno-asm","-O2", "-Wall",
			"-lm", "--static", "-std=c++11", "-DONLINE_JUDGE", "-o", "Main", "Main.cc", NULL };
	const char * CP_P[] =
			{ "fpc", "Main.pas","-Cs32000000","-Sh", "-O2", "-Co", "-Ct", "-Ci", NULL };

	const char * CP_R[] = { "ruby", "-c", "Main.rb", NULL };
	const char * CP_B[] = { "chmod", "+rx", "Main.sh", NULL };
	const char * CP_PH[] = { "php", "-l", "Main.php", NULL };
	const char * CP_PL[] = { "perl", "-c", "Main.pl", NULL };
	const char * CP_CS[] = { "gmcs", "-warn:0", "Main.cs", NULL };
	const char * CP_OC[] = { "gcc", "-o", "Main", "Main.m",
			"-fconstant-string-class=NSConstantString", "-I",
			"/usr/include/GNUstep/", "-L", "/usr/lib/GNUstep/Libraries/",
			"-lobjc", "-lgnustep-base", NULL };
	const char * CP_BS[] = { "fbc","-lang","qb", "Main.bas", NULL };
	const char * CP_CLANG[]={"clang", "Main.c", "-o", "Main", "-fno-asm", "-Wall",
	         		"-lm", "--static", "-std=c99", "-DONLINE_JUDGE", NULL };
	const char * CP_CLANG_CPP[]={"clang++", "Main.cc", "-o", "Main", "-fno-asm", "-Wall",
	         		"-lm", "--static", "-std=c++0x",  "-DONLINE_JUDGE", NULL };
	const char * CP_LUA[] = { "luac","-o","Main", "Main.lua", NULL };
	const char * CP_GO[] = { "go","build","-o","Main","Main.go", NULL };

	char javac_buf[7][32];
	char *CP_J[7];

	for (int i = 0; i < 7; i++)
		CP_J[i] = javac_buf[i];

	//设置java虚拟机的jvm
	sprintf(CP_J[0], "javac");
	/**
	第一组配置参数：-Xms 、-Xmx、-XX:newSize、-XX:MaxnewSize、-Xmn

	1、-Xms ：表示java虚拟机堆区内存初始内存分配的大小，通常为操作系统可用内存的1/64大小即可，
	但仍需按照实际情况进行分配。有可能真的按照这样的一个规则分配时，设计出的软件还没有能够运
	行得起来就挂了。
	2、-Xmx： 表示java虚拟机堆区内存可被分配的最大上限，通常为操作系统可用内存的1/4大小。
	但是开发过程中，通常会将 -Xms 与 -Xmx两个参数的配置相同的值，其目的是为了能够在java垃圾
	回收机制清理完堆区后不需要重新分隔计算堆区的大小而浪费资源。
	*/
	sprintf(CP_J[1], "-J%s", java_xms);//堆区内存分配的大小
	sprintf(CP_J[2], "-J%s", java_xmx);//堆区分配内存的最大上限值

	//设置编码格式
	sprintf(CP_J[3], "-encoding");
	sprintf(CP_J[4], "UTF-8");
	sprintf(CP_J[5], "Main.java");
	CP_J[6] = (char *) NULL;

   //创建判题子进程
	pid = fork();

	//子进程运行，设置资源限制
	if (pid == 0) {
		struct rlimit LIM;
		int cpu=6;
		if (lang==3) cpu=30;
		LIM.rlim_max = cpu;
		LIM.rlim_cur = cpu;
		setrlimit(RLIMIT_CPU, &LIM);
		/**
		unsigned int alarm（unsigned int seconds);

		作用:alarm也称为闹钟函数，它可以在进程中设置一个定时器，当定时器指定的时间到时，
		它向进程发送SIGALRM信号。可以设置忽略或者不捕获此信号，如果采用默认方式其动作是
		终止调用该alarm函数的进程。
		返回值:
		成功：如果调用此alarm（）前，进程已经设置了闹钟时间，则返回上一个闹钟时间的剩余时间，否则返回0。
        出错：-1
		*/
		alarm(cpu);
		LIM.rlim_max = 40 * STD_MB;
		LIM.rlim_cur = 40 * STD_MB;
		setrlimit(RLIMIT_FSIZE, &LIM);

		if(lang==3||lang==17){
#ifdef __i386
		   LIM.rlim_max = STD_MB <<11;
		   LIM.rlim_cur = STD_MB <<11;	
#else
		   LIM.rlim_max = STD_MB <<12;
		   LIM.rlim_cur = STD_MB <<12;	
#endif
                }else{
		   LIM.rlim_max = STD_MB *512 ;
		   LIM.rlim_cur = STD_MB *512 ;
		}
		setrlimit(RLIMIT_AS, &LIM);
		//标准流重定向输出到ce.txt
		if (lang != 2 && lang != 11) {

			/**
			函数，以指定模式重新指定到另一个文件。模式用于指定新文件的访问方式。
			FILE *freopen( const char *filename, const char *mode, FILE *stream );
			filename：需要重定向到的文件名或文件路径。
            mode：代表文件访问权限的字符串。例如，"r"表示“只读访问”、"w"表示“只写访问”、"a"表示“追加写入”。
            stream：需要被重定向的文件流。
            返回值：如果成功，则返回该指向该输出流的文件指针，否则返回为NULL。
			*/
			freopen("ce.txt", "w", stderr);//将错误流重定向到文件
		} else {
			freopen("ce.txt", "w", stdout);
		}

		if(compile_chroot&&lang != 3 && lang != 9 && lang != 6 && lang != 11){

			//创建目录
			execute_cmd("mkdir -p bin usr lib lib64 etc/alternatives proc tmp dev");
			//分配权限
			execute_cmd("chown judge *");
			
			//挂载文件系统
                	execute_cmd("mount -o bind /bin bin");
                	execute_cmd("mount -o bind /usr usr");
                	execute_cmd("mount -o bind /lib lib");
#ifndef __i386
                	execute_cmd("mount -o bind /lib64 lib64");
#endif
                	execute_cmd("mount -o bind /etc/alternatives etc/alternatives");
                	execute_cmd("mount -o bind /proc proc");

                	if(lang>2 && lang!=10 && lang!=13 && lang!=14)
				execute_cmd("mount -o bind /dev dev");
			//chroot(PATH)这个function必须具有 root的身份才能执行，执行后会将根目录切换到PATH 所指定的地方。
                        chroot(work_dir);
		}
		/**
		setuid和setgid位是让普通用户可以以root用户的角色运行只有root帐号才能运行的程序或命令。
		在程序中如果某些地方需要临时使用root权限，可以通过以下步骤实现
     1、修改可执行程序文件所有者为root 命令：chown root filename
     2、修改可执行文件suid位   命令：chmod u+s filename
     3、在程序代码中一开始设置euid为uid
		uid_t ruid ,euid,suid;
		getresuid(&ruid，&euid，&suid);
		setresuid（ruid，ruid，suid）;
     4、需要用到权限的地方
	 uid_t ruid ,euid,suid;
     getresuid(&ruid，&euid，&suid);
	 setresuid（ruid，suid，suid）
	   //需要权限的代码
		getresuid(&ruid，&euid，&suid);
		setresuid（ruid，ruid，suid）
		通过以上四步，基本上就是实现临时获取root权限又不影响安全性,
		其中 ruid为执行用户id，euid为有效id，suid为保存设置id，权限
		的变换靠的就是这个保存设置id。
		*/

		//临时获取root权限又不影响安全性
		while(setgid(1536)!=0) sleep(1);
                while(setuid(1536)!=0) sleep(1);
                while(setresuid(1536, 1536, 1536)!=0) sleep(1);

		switch (lang) {
		case 0:
		    //寻找相应的环境参数值寻找文件并执行
			/**
			execvp（执行文件）
         相关函数 fork，execl，execle，execlp，execv，execve
         表头文件 #include<unistd.h>
         定义函数 int execvp(const char *file ,char * const argv []);
         函数说明 execvp()会从PATH 环境变量所指的目录中查找符合参数file 的文件名，找到后便执行该文件，然后将第二个参数argv传给该欲执行的文件。
         返回值 如果执行成功则函数不会返回，执行失败则直接返回-1，失败原因存于errno中。
			*/
			execvp(CP_C[0], (char * const *) CP_C);
			break;
		case 1:
			execvp(CP_X[0], (char * const *) CP_X);
			break;
		case 2:
			execvp(CP_P[0], (char * const *) CP_P);
			break;
		case 3:
			execvp(CP_J[0], (char * const *) CP_J);
			break;
		case 4:
			execvp(CP_R[0], (char * const *) CP_R);
			break;
		case 5:
			execvp(CP_B[0], (char * const *) CP_B);
			break;
		case 7:
			execvp(CP_PH[0], (char * const *) CP_PH);
			break;
		case 8:
			execvp(CP_PL[0], (char * const *) CP_PL);
			break;
		case 9:
			execvp(CP_CS[0], (char * const *) CP_CS);
			break;

		case 10:
			execvp(CP_OC[0], (char * const *) CP_OC);
			break;
		case 11:
			execvp(CP_BS[0], (char * const *) CP_BS);
			break;
		case 13:
			execvp(CP_CLANG[0], (char * const *) CP_CLANG);
			break;
		case 14:
			execvp(CP_CLANG_CPP[0], (char * const *) CP_CLANG_CPP);
			break;
		case 15:
			execvp(CP_LUA[0], (char * const *) CP_LUA);
			break;
		case 17:
			execvp(CP_GO[0], (char * const *) CP_GO);
			break;
		default:
			printf("nothing to do!\n");
		}
		//debug模式
		if (DEBUG)
			printf("compile end!\n");
		exit(0);
	} 
    else {
	   //当前处于父进程
		int status = 0;

		//等待子进程结束,结束之后卸载文件系统
		waitpid(pid, &status, 0);

		if (lang > 3 && lang < 7)
			status = get_file_size("ce.txt");

		if (DEBUG)
			printf("status=%d\n", status);

		//卸载文件系统
		execute_cmd("/bin/umount -f bin usr lib lib64 etc/alternatives proc dev 2>&1 >/dev/null");
 		execute_cmd("/bin/umount -f %s/* 2>&1 >/dev/null",work_dir);
		umount(work_dir);

		//返回当前的状态
		return status;
	}
}
//获取进程的状态
int get_proc_status(int pid, const char * mark) {
	FILE * pf;
	char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
	int ret = 0;
	sprintf(fn, "/proc/%d/status", pid);
	pf = fopen(fn, "re");

	//获取从字符数组的长度
	int m = strlen(mark);
	while (pf && fgets(buf, BUFFER_SIZE - 1, pf)) {

		buf[strlen(buf) - 1] = 0;

		//比较前m个字符
		if (strncmp(buf, mark, m) == 0) {
			sscanf(buf + m + 1, "%d", &ret);
		}
	}
	if (pf)
		fclose(pf);
	return ret;
}

#ifdef _mysql_h
//初始化数据库连接
int init_mysql_conn() {
	conn = mysql_init(NULL);
	//时间延迟设置为30
	const char timeout = 30;
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

	//连接不成功
	if (!mysql_real_connect(conn, host_name, user_name, password, db_name,
			port_number, 0, 0)) {
		write_log("%s", mysql_error(conn));
		return 0;
	}
	//设置编码格式
	const char * utf8sql = "set names utf8";
	if (mysql_real_query(conn, utf8sql, strlen(utf8sql))) {
		write_log("%s", mysql_error(conn));
		return 0;
	}
	return 1;
}
#endif

#ifdef _mysql_h
//查询数据库获取用户源码并在默认目录work_dir : /home/judge/run0下建立 Main.c
void _get_solution_mysql(int solution_id, char * work_dir, int lang) {
	char sql[BUFFER_SIZE], src_pth[BUFFER_SIZE];

	//获取数据库数据的固定写法
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(sql, "SELECT source FROM source_code WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);

	// 创建src文件目录
	sprintf(src_pth, "Main.%s", lang_ext[lang]);

	if (DEBUG)
		printf("Main=%s", src_pth);

	FILE *fp_src = fopen(src_pth, "we");
	fprintf(fp_src, "%s", row[0]);

	if(res!=NULL) {
	    //释放内存
		mysql_free_result(res);
		res=NULL;
	}
	fclose(fp_src);
}
#endif
//http方式获取源码
void _get_solution_http(int solution_id, char * work_dir, int lang) {
	char src_pth[BUFFER_SIZE];
	//创建src目录文件
	sprintf(src_pth, "Main.%s", lang_ext[lang]);
	if (DEBUG)
		printf("Main=%s", src_pth);

	const char * cmd2 =
			"wget --post-data=\"getsolution=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O %s \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd2, solution_id, src_pth, http_baseurl);

	pclose(pout);
}
void get_solution(int solution_id, char * work_dir, int lang) {
	char src_pth[BUFFER_SIZE];
	sprintf(src_pth, "Main.%s", lang_ext[lang]);
	if (http_judge) {
		_get_solution_http(solution_id, work_dir, lang);
	} else {

#ifdef _mysql_h
		_get_solution_mysql(solution_id, work_dir, lang);
#endif
	}
	execute_cmd("chown judge %s/%s", work_dir,src_pth);
}
#ifdef _mysql_h
//数据库方式获取源码
void _get_custominput_mysql(int solution_id, char * work_dir) {
	char sql[BUFFER_SIZE], src_pth[BUFFER_SIZE];
	//获取用户源码
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(sql, "SELECT input_text FROM custominput WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	if (row != NULL) {

		//生成src目录文件
		sprintf(src_pth, "data.in");
		FILE *fp_src = fopen(src_pth, "w");
		fprintf(fp_src, "%s", row[0]);
		fclose(fp_src);

	}
	if(res!=NULL) {
	//释放内存
		mysql_free_result(res);
		res=NULL;
	}
}
#endif
//http方式获取源码
void _get_custominput_http(int solution_id, char * work_dir) {
	char src_pth[BUFFER_SIZE];

	// 创建src文件
	sprintf(src_pth, "data.in");

	const char * cmd2 =
			"wget --post-data=\"getcustominput=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O %s \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd2, solution_id, src_pth, http_baseurl);
	pclose(pout);

}

//获取用户输入,参数1:判题的id,参数2:特定的工作目录
void get_custominput(int solution_id, char * work_dir) {
	//区分两种不同的获取方式
	if (http_judge) {
		_get_custominput_http(solution_id, work_dir);
	} else {
#ifdef _mysql_h
		_get_custominput_mysql(solution_id, work_dir);
#endif
	}
}

/**获取评测列表方式*/
#ifdef _mysql_h
//数据库方式
void _get_solution_info_mysql(int solution_id, int & p_id, char * user_id,
		int & lang) {

	MYSQL_RES *res;
	MYSQL_ROW row;

	char sql[BUFFER_SIZE];
	// get the problem id and user id from Table:solution
	if(turbo_mode==2){
		sprintf(sql,
				"insert into solution2 select *  FROM solution where solution_id=%d",
				solution_id);
		//printf("%s\n",sql);
		mysql_real_query(conn, sql, strlen(sql));
		sprintf(sql,
				"SELECT problem_id, user_id, language FROM solution2 where solution_id=%d",
				solution_id);
		//printf("%s\n",sql);
	}else{
		sprintf(sql,
				"SELECT problem_id, user_id, language FROM solution where solution_id=%d",
				solution_id);
	}
	//printf("%s\n",sql);
	mysql_real_query(conn, sql, strlen(sql));

	/**
	MYSQL_RES *mysql_store_result(MYSQL *mysql)
	具有多个结果的MYSQL_RES结果集合。如果出现错误，返回NULL。
	*/
	res = mysql_store_result(conn);

	//该函数返回的是一个数组
	row = mysql_fetch_row(res);

	//当前的问题id,字符串转换成整型
	p_id = atoi(row[0]);

	//复制当前的用户id
	strcpy(user_id, row[1]);

	//当前使用的语言
	lang = atoi(row[2]);
	
	//直接释放内存
	if(res!=NULL) {
		mysql_free_result(res);
		res=NULL;//指针设置为空
	}
}
#endif
//http方式
void _get_solution_info_http(int solution_id, int & p_id, char * user_id,
		int & lang) {

	login();
	const char * cmd =
			"wget --post-data=\"getsolutioninfo=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd, solution_id, http_baseurl);
	fscanf(pout, "%d", &p_id);
	fscanf(pout, "%s", user_id);
	fscanf(pout, "%d", &lang);
	pclose(pout);

}
//获取待判题对象
void get_solution_info(int solution_id, int & p_id, char * user_id,
		int & lang) {

	if (http_judge) {
		_get_solution_info_http(solution_id, p_id, user_id, lang);
	} else {
#ifdef _mysql_h
		_get_solution_info_mysql(solution_id, p_id, user_id, lang);
#endif
	}
}

#ifdef _mysql_h
void _get_problem_info_mysql(int p_id, int & time_lmt, int & mem_lmt,
		int & isspj) {
	char sql[BUFFER_SIZE];//存储格式化之后的sql语句

	MYSQL_RES *res;
	MYSQL_ROW row;
	//查询数据库相关字段
	sprintf(sql,
			"SELECT time_limit,memory_limit,spj FROM problem where problem_id=%d",
			p_id);

	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);

	//获取当前的进程资源限制
	time_lmt = atoi(row[0]);
	mem_lmt = atoi(row[1]);

	isspj = (row[2][0] == '1');
	if(res!=NULL) {
		mysql_free_result(res);                         // free the memory
		res=NULL;
	}
}
#endif
void _get_problem_info_http(int p_id, int & time_lmt, int & mem_lmt,
		int & isspj) {
	//login();
	const char * cmd =
			"wget --post-data=\"getprobleminfo=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd, p_id, http_baseurl);
	fscanf(pout, "%d", &time_lmt);
	fscanf(pout, "%d", &mem_lmt);
	fscanf(pout, "%d", &isspj);
	pclose(pout);
}

void get_problem_info(int p_id, int & time_lmt, int & mem_lmt, int & isspj) {
	if (http_judge) {
		_get_problem_info_http(p_id, time_lmt, mem_lmt, isspj);
	} else {
#ifdef _mysql_h
		_get_problem_info_mysql(p_id, time_lmt, mem_lmt, isspj);
#endif
	}
	if(time_lmt<=0) time_lmt=1;
	
}


char *escape(char s[], char t[])
{
    int i, j;
    for(i = j = 0; t[i] != '\0'; ++i){
	if(t[i]=='\'') {
		s[j++]='\'';
		s[j++]='\\';
		s[j++]='\'';
		s[j++]='\'';
		continue;
	}else{
            s[j++] = t[i];
        }
    }
    s[j] = '\0';
    return s;
}
//outfile:正确结果的数据
// userfile：用户程序数据
void prepare_files(char * filename, int namelen, char * infile, int & p_id,
		char * work_dir, char * outfile, char * userfile, int runner_id)
		{
		char fname0[BUFFER_SIZE];
		char fname[BUFFER_SIZE];
		//前namelen个长度的字符串复制到fname0
		strncpy(fname0, filename, namelen);

		fname0[namelen] = 0;
		escape(fname,fname0);
		printf("%s\n%s\n",fname0,fname);
		sprintf(infile, "%s/data/%d/%s.in", oj_home, p_id, fname);
		execute_cmd("/bin/cp '%s' %s/data.in", infile, work_dir);
		execute_cmd("/bin/cp %s/data/%d/*.dic %s/", oj_home, p_id, work_dir);
         //正确的输出文件
		sprintf(outfile, "%s/data/%d/%s.out", oj_home, p_id, fname0);
		//用户程序结果数据路径
		sprintf(userfile, "%s/run%d/user.out", oj_home, runner_id);
}

//复制shell的运行时间
void copy_shell_runtime(char * work_dir) {

	execute_cmd("/bin/mkdir %s/lib", work_dir);
	execute_cmd("/bin/mkdir %s/lib64", work_dir);
	execute_cmd("/bin/mkdir %s/bin", work_dir);

#ifdef __i386
        execute_cmd("/bin/cp /lib/ld-linux* %s/lib/", work_dir);
        execute_cmd("/bin/cp -a /lib/i386-linux-gnu  %s/lib/", work_dir);
        execute_cmd("/bin/cp -a /usr/lib/i386-linux-gnu %s/lib/", work_dir);
#endif

	execute_cmd("/bin/cp -a /lib/x86_64-linux-gnu %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib64/* %s/lib64/", work_dir);
//	execute_cmd("/bin/cp /lib32 %s/", work_dir);
	execute_cmd("/bin/cp /bin/busybox %s/bin/", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/sh", work_dir);
	execute_cmd("/bin/cp /bin/bash %s/bin/bash", work_dir);

}
void copy_objc_runtime(char * work_dir) {
	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/proc", work_dir);
	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/lib/", work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libdbus-1.so.3                          %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgcc_s.so.1                           %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgcrypt.so.11                         %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgpg-error.so.0                       %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libz.so.1                               %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libc.so.6                 %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libdl.so.2                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libm.so.6                 %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libnsl.so.1               %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libpthread.so.0           %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/librt.so.1                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libavahi-client.so.3                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libavahi-common.so.3                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libdns_sd.so.1                      %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libffi.so.5                         %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libgnustep-base.so.1.19             %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libgnutls.so.26                     %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libobjc.so.2                        %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libtasn1.so.3                       %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libxml2.so.2                        %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libxslt.so.1                        %s/lib/ ",
			work_dir);

}
void copy_bash_runtime(char * work_dir) {
	//char cmd[BUFFER_SIZE];
	//const char * ruby_run="/usr/bin/ruby";
	copy_shell_runtime(work_dir);
	execute_cmd("/bin/cp `which bc`  %s/bin/", work_dir);
	execute_cmd("busybox dos2unix Main.sh", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/grep", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/awk", work_dir);
	execute_cmd("/bin/cp /bin/sed %s/bin/sed", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/cut", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/sort", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/join", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/wc", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/tr", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/dc", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/dd", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/cat", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/tail", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/head", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/xargs", work_dir);
	execute_cmd("chmod +rx %s/Main.sh", work_dir);

}
void copy_ruby_runtime(char * work_dir) {

        copy_shell_runtime(work_dir);
        execute_cmd("mkdir -p %s/usr", work_dir);
        execute_cmd("mkdir -p %s/usr/lib", work_dir);
        execute_cmd("mkdir -p %s/usr/lib64", work_dir);
        execute_cmd("cp -a /usr/lib/libruby* %s/usr/lib/", work_dir);
        execute_cmd("cp -a /usr/lib/ruby* %s/usr/lib/", work_dir);
        execute_cmd("cp -a /usr/lib64/ruby* %s/usr/lib64/", work_dir);
        execute_cmd("cp -a /usr/lib64/libruby* %s/usr/lib64/", work_dir);
        execute_cmd("cp -a /usr/bin/ruby* %s/", work_dir);
        execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libruby* %s/usr/lib/",work_dir);
        execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libgmp* %s/usr/lib/",work_dir);

}
void copy_guile_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/proc", work_dir);
	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/share", work_dir);
	execute_cmd("/bin/cp -a /usr/share/guile %s/usr/share/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libguile* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgc* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libffi* %s/usr/lib/",
			work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libunistring* %s/usr/lib/",
			work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libgmp* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgmp* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libltdl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libltdl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/bin/guile* %s/", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libguile* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libgc* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libffi* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp -a /usr/lib/x86_64-linux-gnu/libunistring* %s/usr/lib/",work_dir);

}
void copy_python_runtime(char * work_dir) {

        copy_shell_runtime(work_dir);
        execute_cmd("mkdir -p %s/usr/include", work_dir);
        execute_cmd("mkdir -p %s/dev", work_dir);
        execute_cmd("mkdir -p %s/usr/lib", work_dir);
        execute_cmd("mkdir -p %s/usr/lib64", work_dir);
        execute_cmd("mkdir -p %s/usr/local/lib", work_dir);
        execute_cmd("cp /usr/bin/python* %s/", work_dir);
        execute_cmd("cp -a /usr/lib/python* %s/usr/lib/", work_dir);
        execute_cmd("cp -a /usr/lib64/python* %s/usr/lib64/", work_dir);
        execute_cmd("cp -a /usr/local/lib/python* %s/usr/local/lib/", work_dir);
        execute_cmd("cp -a /usr/include/python* %s/usr/include/", work_dir);
        execute_cmd("cp -a /usr/lib/libpython* %s/usr/lib/", work_dir);
        execute_cmd("/bin/mkdir -p %s/home/judge", work_dir);
	    execute_cmd("/bin/chown judge %s", work_dir);
	    execute_cmd("/bin/mkdir -p %s/etc", work_dir);
	    execute_cmd("/bin/grep judge /etc/passwd>%s/etc/passwd", work_dir);
	    execute_cmd("/bin/mount -o bind /dev %s/dev", work_dir);


}
void copy_php_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/lib/libedit* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libdb* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgssapi_krb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libkrb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libk5crypto* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libedit* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libdb* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libgssapi_krb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libkrb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libk5crypto* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libxml2* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libxml2.so* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libicuuc.so* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libicudata.so* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libstdc++.so* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libssl* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libcrypto* %s/usr/lib/",work_dir);
	execute_cmd("/bin/cp /usr/bin/php* %s/", work_dir);
	execute_cmd("chmod +rx %s/Main.php", work_dir);
	

}
void copy_perl_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/lib/libperl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/bin/perl* %s/", work_dir);

}
void copy_freebasic_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/lib", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/bin", work_dir);
	execute_cmd("/bin/cp /usr/local/lib/freebasic %s/usr/local/lib/", work_dir);
	execute_cmd("/bin/cp /usr/local/bin/fbc %s/", work_dir);
	execute_cmd("/bin/cp -a /lib32/* %s/lib/", work_dir);

}
void copy_mono_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib/mono/2.0", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/mono %s/usr/lib/", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib64/mono/2.0", work_dir);
	execute_cmd("/bin/cp -a /usr/lib64/mono %s/usr/lib64/", work_dir);

	execute_cmd("/bin/cp /usr/lib/libgthread* %s/usr/lib/", work_dir);

	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/cp /usr/bin/mono* %s/", work_dir);

	execute_cmd("/bin/cp /usr/lib/libgthread* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /lib/libglib* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib/tls/i686/cmov/lib* %s/lib/tls/i686/cmov/",
			work_dir);
	execute_cmd("/bin/cp /lib/libpcre* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib/ld-linux* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib64/ld-linux* %s/lib64/", work_dir);
	execute_cmd("/bin/mkdir -p %s/home/judge", work_dir);
	execute_cmd("/bin/chown judge %s/home/judge", work_dir);
	execute_cmd("/bin/mkdir -p %s/etc", work_dir);
	execute_cmd("/bin/grep judge /etc/passwd>%s/etc/passwd", work_dir);

}
void copy_lua_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/lib", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/bin", work_dir);
	execute_cmd("/bin/cp /usr/bin/lua %s/", work_dir);

}
void copy_js_runtime(char * work_dir) {

	//copy_shell_runtime(work_dir);
        execute_cmd("mkdir -p %s/dev", work_dir);
	execute_cmd("/bin/mount -o bind /dev %s/dev", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib %s/lib/i386-linux-gnu/ %s/lib64/", work_dir, work_dir, work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libz.so.*  %s/lib/i386-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libuv.so.*  %s/lib/i386-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libicui18n.so.*  %s/lib/i386-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libicuuc.so.*  %s/lib/i386-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libicudata.so.*  %s/lib/i386-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/i386-linux-gnu/libtinfo.so.*  %s/lib/i386-linux-gnu/", work_dir);
	
        execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libcares.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /usr/lib/libv8.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libssl.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libcrypto.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libdl.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/librt.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libstdc++.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libpthread.so.*  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libc.so.6  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libm.so.6  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/i386-linux-gnu/libgcc_s.so.1  %s/lib/i386-linux-gnu/", work_dir);
        execute_cmd("/bin/cp /lib/ld-linux.so.*  %s/lib/", work_dir);

	execute_cmd("/bin/mkdir -p %s/usr/lib %s/lib/x86_64-linux-gnu/", work_dir, work_dir);
	
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libz.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libuv.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/librt.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libpthread.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libdl.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libssl.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libcrypto.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libicui18n.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libicuuc.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libstdc++.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libm.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libgcc_s.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib/x86_64-linux-gnu/libc.so.* %s/lib/x86_64-linux-gnu/", work_dir);
	execute_cmd("/bin/cp /lib64/ld-linux-x86-64.so.* %s/lib64/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libicudata.so.* %s/lib/x86_64-linux-gnu/", work_dir);


	execute_cmd("/bin/cp /usr/bin/nodejs %s/", work_dir);

}

//运行
void run_solution(int & lang, char * work_dir, int & time_lmt, int & usedtime,
		int & mem_lmt) {
	//设置进程的优先级
	nice(19);
	int py2=execute_cmd("/bin/grep 'python2' Main.py");
	// 确定当前工作目录
	chdir(work_dir);
	// 重定向文到此处，方便后面最后结果进行评判
	freopen("data.in", "r", stdin);
	freopen("user.out", "w", stdout);
	freopen("error.out", "a+", stderr);
	// 进程跟踪
	if(use_ptrace) ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	// run me
	if (lang != 3)

		chroot(work_dir);

	while (setgid(1536) != 0)
		sleep(1);
	while (setuid(1536) != 0)
		sleep(1);
	while (setresuid(1536, 1536, 1536) != 0)
		sleep(1);

	// 子进程
	struct rlimit LIM;
	// 时间限制
	if (oi_mode)
		LIM.rlim_cur = time_lmt/cpu_compensation + 1;
	else
		LIM.rlim_cur = (time_lmt/cpu_compensation  - usedtime / 1000) + 1;
	LIM.rlim_max = LIM.rlim_cur;
	setrlimit(RLIMIT_CPU, &LIM);
	alarm(0);
	alarm(time_lmt * 5 / cpu_compensation);

	// 文件大小限制
	LIM.rlim_max = STD_F_LIM + STD_MB;
	LIM.rlim_cur = STD_F_LIM;
	setrlimit(RLIMIT_FSIZE, &LIM);
	//虚拟内存限制
	switch (lang) {
	case 17: 
	case 9: //C#	
		LIM.rlim_cur = LIM.rlim_max = 280;
		break;
	case 3:  //java
	case 4:  //ruby
	case 12:
	case 16:
		LIM.rlim_cur = LIM.rlim_max = 80;
		break;
	case 5: //bash
		LIM.rlim_cur = LIM.rlim_max = 3;
		break;
	default:
		LIM.rlim_cur = LIM.rlim_max = 1;
	}

	setrlimit(RLIMIT_NPROC, &LIM);

	//栈大小的限制
	LIM.rlim_cur = STD_MB << 6;
	LIM.rlim_max = STD_MB << 6;
	setrlimit(RLIMIT_STACK, &LIM);
	// 内存限制
	LIM.rlim_cur = STD_MB * mem_lmt / 2 * 3;
	LIM.rlim_max = STD_MB * mem_lmt * 2;
	if (lang < 3)
	//设置最大虚内存空间
		setrlimit(RLIMIT_AS, &LIM);

	switch (lang) {
	case 0:
	case 1:
	case 2:
	case 10:
	case 11:
	case 13:
	case 14:
	case 17:
		execl("./Main", "./Main", (char *) NULL);
		break;
	case 3:
	          //java_xms设置内存使用的最大数量
              sprintf(java_xms, "-Xmx%dM", mem_lmt);
		execl("/usr/bin/java", "/usr/bin/java", java_xms, java_xmx,
				"-Djava.security.manager",
				"-Djava.security.policy=./java.policy", "Main", (char *) NULL);
		break;
	case 4: //ruby
		execl("/ruby", "/ruby", "Main.rb", (char *) NULL);
		break;
	case 5: //bash
		execl("/bin/bash", "/bin/bash", "Main.sh", (char *) NULL);
		break;
	case 6: //Python
		if(!py2){	
			execl("/python2", "/python2", "Main.py", (char *) NULL);
		}else{
			execl("/python3", "/python3", "Main.py", (char *) NULL);
		}
		break;
	case 7: //php
		execl("/php", "/php", "Main.php", (char *) NULL);
		break;
	case 8: //perl
		execl("/perl", "/perl", "Main.pl", (char *) NULL);
		break;
	case 9: //Mono C#
		execl("/mono", "/mono", "--debug", "Main.exe", (char *) NULL);
		break;
	case 12: //guile
		execl("/guile", "/guile", "Main.scm", (char *) NULL);
		break;
	case 15: //guile
		execl("/lua", "/lua", "Main", (char *) NULL);
		break;
	case 16: //Node.js
		execl("/nodejs", "/nodejs", "Main.js", (char *) NULL);
		break;

	}
	fflush(stderr);
	exit(0);
}

/**
Java和pathon的特殊处理
*/
int fix_python_mis_judge(char *work_dir, int & ACflg, int & topmemory,
                int mem_lmt) {
        int comp_res = OJ_AC;

        comp_res = execute_cmd(
                       //grep命令查找memoryerror
                        "/bin/grep 'MemoryError'  %s/error.out", work_dir);

        if (!comp_res) {
                printf("Python need more Memory!");
                ACflg = OJ_ML;
                topmemory = mem_lmt * STD_MB;
        }
        return comp_res;
}

int fix_java_mis_judge(char *work_dir, int & ACflg, int & topmemory,
		int mem_lmt) {
	int comp_res = OJ_AC;
	execute_cmd("chmod 700 %s/error.out", work_dir);
	if (DEBUG)
		execute_cmd("cat %s/error.out", work_dir);
	comp_res = execute_cmd("/bin/grep 'Exception'  %s/error.out", work_dir);
	if (!comp_res) {
		printf("Exception reported\n");
		//运行错误
		ACflg = OJ_RE;
	}
	execute_cmd("cat %s/error.out", work_dir);

	comp_res = execute_cmd(
			"/bin/grep 'java.lang.OutOfMemoryError'  %s/error.out", work_dir);

	if (!comp_res) {
		printf("JVM need more Memory!");
		ACflg = OJ_ML;
		topmemory = mem_lmt * STD_MB;
	}

	if (!comp_res) {
		printf("JVM need more Memory or Threads!");
		ACflg = OJ_ML;
		topmemory = mem_lmt * STD_MB;
	}
	comp_res = execute_cmd("/bin/grep 'Could not create'  %s/error.out",
			work_dir);
	if (!comp_res) {
		printf("jvm need more resource,tweak -Xmx(OJ_JAVA_BONUS) Settings");
		ACflg = OJ_RE;
	}
	return comp_res;
}
/**int special_judge(char* oj_home, int problem_id, char *infile, char *outfile,
		char *userfile) {

	pid_t pid;
	printf("pid=%d\n", problem_id);
	pid = fork();
	int ret = 0;
	if (pid == 0) {

		while (setgid(1536) != 0)
			sleep(1);
		while (setuid(1536) != 0)
			sleep(1);
		while (setresuid(1536, 1536, 1536) != 0)
			sleep(1);

		struct rlimit LIM;

		LIM.rlim_cur = 5;
		LIM.rlim_max = LIM.rlim_cur;
		setrlimit(RLIMIT_CPU, &LIM);
		alarm(0);
		alarm(10);

		// 文件大小限制
		LIM.rlim_max = STD_F_LIM + STD_MB;
		LIM.rlim_cur = STD_F_LIM;
		setrlimit(RLIMIT_FSIZE, &LIM);

		ret = execute_cmd("%s/data/%d/spj '%s' '%s' %s", oj_home, problem_id,
				infile, outfile, userfile);
		if (DEBUG)
			printf("spj1=%d\n", ret);
		if (ret)
			exit(1);
		else
			exit(0);
	} else {
		int status;

		waitpid(pid, &status, 0);
		ret = WEXITSTATUS(status);
		if (DEBUG)
			printf("spj2=%d\n", ret);
	}
	return ret;
}*/
//评判开始
void judge_solution(int & ACflg, int & usedtime, int time_lmt, int isspj,
		int p_id, char * infile, char * outfile, char * userfile, int & PEflg,
		int lang, char * work_dir, int & topmemory, int mem_lmt,
		int solution_id, int num_of_test) {
	int comp_res;
	if (!oi_mode)
		num_of_test = 1.0;

	//程序正确但是超时间
	if (ACflg == OJ_AC && usedtime > time_lmt * 1000 * (use_max_time ? 1 : num_of_test))
		ACflg = OJ_TL;

	//超内存
	if (topmemory > mem_lmt * STD_MB)
		ACflg = OJ_ML;

	// 校验数据
	if (ACflg == OJ_AC) {
		if (isspj) {
			comp_res = special_judge(oj_home, p_id, infile, outfile, userfile);
			if (comp_res == 0)
				comp_res = OJ_AC;
			else {
				if (DEBUG)
					printf("fail test %s\n", infile);
				comp_res = OJ_WA;
			}
		} else {
		//比较两个输出文件，宏作为返回值
			comp_res = compare(outfile, userfile);
		}
		if (comp_res == OJ_WA) {
			ACflg = OJ_WA;
			if (DEBUG)
				printf("fail test %s\n", infile);
		} else if (comp_res == OJ_PE)
			PEflg = OJ_PE;
		ACflg = comp_res;
	}
     //Java、Python 都是解释型语言，特殊处理
	if (lang == 3) {
		comp_res = fix_java_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
	}
	if (lang == 6) {
		comp_res = fix_python_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
	}
}

int get_page_fault_mem(struct rusage & ruse, pid_t & pidApp) {
	//java use pagefault
	int m_vmpeak, m_vmdata, m_minflt;
	m_minflt = ruse.ru_minflt * getpagesize();
	if (0 && DEBUG) {
		m_vmpeak = get_proc_status(pidApp, "VmPeak:");
		m_vmdata = get_proc_status(pidApp, "VmData:");
		printf("VmPeak:%d KB VmData:%d KB minflt:%d KB\n", m_vmpeak, m_vmdata,
				m_minflt >> 10);
	}
	return m_minflt;
}
//运行错误信息打印
void print_runtimeerror(char * err) {
	//追加的形式打开文件
	FILE *ferr = fopen("error.out", "a+");
	fprintf(ferr, "Runtime Error:%s\n", err);
	fclose(ferr);
}

//参数传入一个进程id号，用作执行命令行的参数
void clean_session(pid_t p) {
	const char *pre = "ps awx -o \"\%p \%P\"|grep -w ";
	const char *post = " | awk \'{ print $1  }\'|xargs kill -9";
	execute_cmd("%s %d %s", pre, p, post);
	execute_cmd("ps aux |grep \\^judge|awk '{print $2}'|xargs kill");
}
//跟踪调试子程序
void watch_solution(pid_t pidApp, char * infile, int & ACflg, int isspj,
		char * userfile, char * outfile, int solution_id, int lang,
		int & topmemory, int mem_lmt, int & usedtime, int time_lmt, int & p_id,
		int & PEflg, char * work_dir) {
	
	// parent
	int tempmemory=0;//当前的内存大小

	if (DEBUG)
		printf("pid=%d judging %s\n", pidApp, infile);

	int status, sig, exitcode;
	struct user_regs_struct reg;
	struct rusage ruse;
	
	//标志位
	int first = true;
	while (1) {
		// check the usage
		wait4(pidApp, &status, __WALL, &ruse);
		if(first){
			ptrace(PTRACE_SETOPTIONS, pidApp, NULL, PTRACE_O_TRACESYSGOOD 
								|PTRACE_O_TRACEEXIT);
		} 
		if (lang == 3 || lang == 7 || lang == 16 || lang==9 ||lang==17) {
			tempmemory = get_page_fault_mem(ruse, pidApp);
		} else {
		//VmPeak表示当前进程运行过程中占用内存的峰值.
			tempmemory = get_proc_status(pidApp, "VmPeak:") << 10;
		}
		//设置当前的最高内存值
		if (tempmemory > topmemory)
			topmemory = tempmemory;

		//当前超出内存，标记ACflag为OJ_ML
		if (topmemory > mem_lmt * STD_MB) {
			if (DEBUG)
				printf("out of memory %d\n", topmemory);
			if (ACflg == OJ_AC)
				ACflg = OJ_ML;

			//杀死跟踪进程
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}
		//sig = status >> 8;/*status >> 8 EXITCODE*/

		if (WIFEXITED(status))
			break;
		if ((lang < 4 || lang == 9) && get_file_size("error.out") && !oi_mode) {
			//运行时错误
			ACflg = OJ_RE;
			//终止跟踪进程
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}

		if (!isspj
				&& get_file_size(userfile)
						> get_file_size(outfile) * 2 + 1024) {
			//死循环导致答案过长
			ACflg = OJ_OL;
			//杀死跟踪进程
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}
		//进程的退出状态
		exitcode = WEXITSTATUS(status);
         //等待下一个cpu的分配
		if ((lang >= 3 && exitcode == 17) || exitcode == 0x05 || exitcode == 0 || exitcode == 133 )
			;
		else {
			//debug状态下打印退出码
			if (DEBUG) {
				printf("status>>8=%d\n", exitcode);
			}
			//状态为接受状态
			if (ACflg == OJ_AC) {
				switch (exitcode) {
				case SIGCHLD:

				case SIGALRM: //超时，打印超时时间
					alarm(0);
					if(DEBUG) printf("alarm:%d\n",time_lmt);
				case SIGKILL:

				case SIGXCPU:
					ACflg = OJ_TL;
					usedtime=time_lmt*1000;
					//打印使用时间
					if(DEBUG) printf("TLE:%d\n",usedtime);
					break;

				case SIGXFSZ://死循环造成答案过长
					ACflg = OJ_OL;
					break;
				default:  //默认是运行错误
					ACflg = OJ_RE;
				}
				print_runtimeerror(strsignal(exitcode));
			}
			//杀死跟踪进程
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}
		//进程被信号中止
		if (WIFSIGNALED(status)) {
			/*  WIFSIGNALED: if the process is terminated by signal
			 *
			 *  psignal(int sig, char *s)，like perror(char *s)，print out s, with error msg from system of sig  
			 * sig = 5 means Trace/breakpoint trap
			 * sig = 11 means Segmentation fault
			 * sig = 25 means File size limit exceeded
			 */

			 //获取子进程被信号终止的信号代码，并根据修改ACflg状态
			sig = WTERMSIG(status);

			if (DEBUG) {
				printf("WTERMSIG=%d\n", sig);
				psignal(sig, NULL);
			}
			if (ACflg == OJ_AC) {
				//表达式是信号，设置判题状态
				switch (sig) {
				case SIGCHLD:
				case SIGALRM:
				//取消之前的闹钟，返回剩下的时间
					alarm(0);
				case SIGKILL:
				case SIGXCPU:
					ACflg = OJ_TL;
					break;
				case SIGXFSZ:
					ACflg = OJ_OL;
					break;
				default:
					ACflg = OJ_RE;
				}
				print_runtimeerror(strsignal(sig));
			}
			break;
		}
		// 监控检查系统调用
		ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);
		call_id=(unsigned int)reg.REG_SYSCALL % call_array_size;

		if (call_counter[call_id] ){

		}else if (record_call) {

			call_counter[call_id] = 1;
		
		}else {
		    //jvm中不要限制系统调用
			ACflg = OJ_RE;
			//字符数组，将错误提示信息格式化写到这里
			char error[BUFFER_SIZE];
			sprintf(error,
              "[ERROR] A Not allowed system call: runid:%d CALLID:%u\n"
              " TO FIX THIS , ask admin to add the CALLID into corresponding LANG_XXV[] located at okcalls32/64.h ,\n"
              "and recompile judge_client. \n"
              "if you are admin and you don't know what to do ,\n"
               "chinese explaination can be found on https://zhuanlan.zhihu.com/p/24498599\n",
                 solution_id, call_id);
 
			write_log(error);
			print_runtimeerror(error);
			 //删除子进程的跟踪器
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
		}
		ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
		first = false;
	}
	//程序运行时间的计算
	usedtime += (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000) * cpu_compensation;
	usedtime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000) * cpu_compensation;
}

void clean_workdir(char * work_dir) {
    //卸载/home/judge/run0下的文件系统 /proc
	umount(work_dir);
 	if (DEBUG) {
 	   //DEBUG ，递归删除日志文件
		execute_cmd("/bin/rm -rf %s/log/*", work_dir);
		execute_cmd("mkdir %s/log/", work_dir);
		execute_cmd("/bin/mv %s/* %s/log/", work_dir, work_dir);
	} else {
	  //不是DEBUG，递归删除/home/judge/run0下所有文件
		execute_cmd("mkdir %s/log/", work_dir);
		execute_cmd("/bin/mv %s/* %s/log/", work_dir, work_dir);
		execute_cmd("/bin/rm -rf %s/log/*", work_dir);
	}
}
//初始化参数
void init_parameters(int argc, char ** argv, int & solution_id,
		int & runner_id) {
		//直接报错写入标准错误流退出
	if (argc < 3) {
		fprintf(stderr, "Usage:%s solution_id runner_id.\n", argv[0]);
		fprintf(stderr, "Multi:%s solution_id runner_id judge_base_path.\n",
				argv[0]);
		fprintf(stderr,
				"Debug:%s solution_id runner_id judge_base_path debug.\n",
				argv[0]);
		exit(1);
	}
	/**
	 char* argv[]
	 1.runner_id
	 2.solution_id
	 3.oj_home
	 4.debug，大于4开启debug
	 5.LANG_NAME
	 大于5：record_call
	*/
	//参数>4开启debug
	DEBUG = (argc > 4);
	record_call = (argc > 5);
	//评测语言
	if (argc > 5) {
		strcpy(LANG_NAME, argv[5]);
	}
	//评测目录
	if (argc > 3)
		strcpy(oj_home, argv[3]);
	else
		strcpy(oj_home, "/home/judge");

	chdir(oj_home); //改变当前的工作目录
    //将第二和第三个参数转换成整型
	solution_id = atoi(argv[1]);
	runner_id = atoi(argv[2]);
}
//代码相似度检测（测试）
/**int get_sim(int solution_id, int lang, int pid, int &sim_s_id) {
	char src_pth[BUFFER_SIZE];
	//char cmd[BUFFER_SIZE];
	sprintf(src_pth, "Main.%s", lang_ext[lang]);

	int sim = execute_cmd("/usr/bin/sim.sh %s %d", src_pth, pid);
	if (!sim) {
		execute_cmd("/bin/mkdir ../data/%d/ac/", pid);

		execute_cmd("/bin/cp %s ../data/%d/ac/%d.%s", src_pth, pid, solution_id,
				lang_ext[lang]);
		//c cpp will
		if (lang == 0)
			execute_cmd("/bin/ln ../data/%d/ac/%d.%s ../data/%d/ac/%d.%s", pid,
					solution_id, lang_ext[lang], pid, solution_id,
					lang_ext[lang + 1]);
		if (lang == 1)
			execute_cmd("/bin/ln ../data/%d/ac/%d.%s ../data/%d/ac/%d.%s", pid,
					solution_id, lang_ext[lang], pid, solution_id,
					lang_ext[lang - 1]);
	} else {
		FILE * pf;
		pf = fopen("sim", "r");
		if (pf) {
			fscanf(pf, "%d%d", &sim, &sim_s_id);
			fclose(pf);
		}
	}
	if (solution_id <= sim_s_id)
		sim = 0;
	return sim;
}*/
/**void mk_shm_workdir(char * work_dir) {
	char shm_path[BUFFER_SIZE];
	sprintf(shm_path, "/dev/shm/hustoj/%s", work_dir);
	execute_cmd("/bin/mkdir -p %s", shm_path);
	execute_cmd("/bin/ln -s %s %s/", shm_path, oj_home);
	execute_cmd("/bin/chown judge %s ", shm_path);
	execute_cmd("chmod 755 %s ", shm_path);
	//sim need a soft link in shm_dir to work correctly
	sprintf(shm_path, "/dev/shm/hustoj/%s/", oj_home);
	execute_cmd("/bin/ln -s %s/data %s", oj_home, shm_path);
}*/
int count_in_files(char * dirpath) {
	const char * cmd = "ls -l %s/*.in|wc -l";
	int ret = 0;
	FILE * fjobs = read_cmd_output(cmd, dirpath);
	fscanf(fjobs, "%d", &ret);
	pclose(fjobs);

	return ret;
}
//获取测试数据文件
int get_test_file(char* work_dir, int p_id) {
	char filename[BUFFER_SIZE];
	char localfile[BUFFER_SIZE];
	int ret = 0;
	const char * cmd =
			" wget --post-data=\"gettestdatalist=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, p_id, http_baseurl);
	while (fgets(filename, BUFFER_SIZE - 1, fjobs) != NULL) {
		sscanf(filename, "%s", filename);
		if(http_judge&&(!data_list_has(filename))) data_list_add(filename);
		sprintf(localfile, "%s/data/%d/%s", oj_home, p_id, filename);
		if (DEBUG)
			printf("localfile[%s]\n", localfile);

		const char * check_file_cmd =
				" wget --post-data=\"gettestdatadate=1&filename=%d/%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O -  \"%s/admin/problem_judge.php\"";
		FILE * rcop = read_cmd_output(check_file_cmd, p_id, filename,
				http_baseurl);
		time_t remote_date, local_date;
		fscanf(rcop, "%ld", &remote_date);
		fclose(rcop);
		struct stat fst;
		stat(localfile, &fst);
		local_date = fst.st_mtime;

		if (access(localfile, 0) == -1 || local_date < remote_date) {
			if (strcmp(filename, "spj") == 0)
				continue;
			execute_cmd("/bin/mkdir -p %s/data/%d", oj_home, p_id);
			const char * cmd2 =
					" wget --post-data=\"gettestdata=1&filename=%d/%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O \"%s\"  \"%s/admin/problem_judge.php\"";
			execute_cmd(cmd2, p_id, filename, localfile, http_baseurl);
			ret++;

			if (strcmp(filename, "spj.c") == 0) {
				if (access(localfile, 0) == 0) {
					const char * cmd3 = "gcc -o %s/data/%d/spj %s/data/%d/spj.c";
					execute_cmd(cmd3, oj_home, p_id, oj_home, p_id);
				}

			}
			if (strcmp(filename, "spj.cc") == 0) {
				if (access(localfile, 0) == 0) {
					const char * cmd4 =
							"g++ -o %s/data/%d/spj %s/data/%d/spj.cc";
					execute_cmd(cmd4, oj_home, p_id, oj_home, p_id);
				}
			}
		}
	}
	pclose(fjobs);
	return ret;
}

void print_call_array() {
	printf("int LANG_%sV[256]={", LANG_NAME);
	int i = 0;
	for (i = 0; i < call_array_size; i++) {
		if (call_counter[i]) {
			printf("%d,", i);
		}
	}
	printf("0};\n");

	printf("int LANG_%sC[256]={", LANG_NAME);
	for (i = 0; i < call_array_size; i++) {
		if (call_counter[i]) {
			printf("HOJ_MAX_LIMIT,");
		}
	}
	printf("0};\n");

}
int main(int argc, char** argv) {

	char work_dir[BUFFER_SIZE]; //工作目录
	char user_id[BUFFER_SIZE]; //用户id
	int solution_id = 1000;
	int runner_id = 0;
	int p_id, time_lmt, mem_lmt, lang, isspj, sim, sim_s_id, max_case_time = 0;

  //初始化参数
	init_parameters(argc, argv, solution_id, runner_id);

	init_mysql_conf();

 //无法连接到数据库
#ifdef _mysql_h
	if (!http_judge && !init_mysql_conn()) {
		exit(0);
	}
#endif
	//设置工作目录并且开始运行，评测
	sprintf(work_dir, "%s/run%s/", oj_home, argv[2]);

	clean_workdir(work_dir);
	if (shm_run)
		mk_shm_workdir(work_dir);

	chdir(work_dir);
	
	if (http_judge)
		system("/bin/ln -s ../cookie ./");
	 //读取数据库solution表依据solution_id,初始化p_id问题id,user_id用户id，lang编程语言编码
	get_solution_info(solution_id, p_id, user_id, lang);
    //默认在子进程中读取时间空间限制
	if (p_id == 0) {
		time_lmt = 5;
		mem_lmt = 128;
		//isspj = 0;
	} else {
	    //父进程则直接获取判题的id
		get_problem_info(p_id, time_lmt, mem_lmt, isspj);
	}
	//源码从数据库中读取到/home/judge/run0/ 建立Main.c
	get_solution(solution_id, work_dir, lang);

	//Java比较特殊
	if (lang >= 3 && lang != 10 && lang != 13 && lang != 14) {
		// java的时空限制
		time_lmt = time_lmt + java_time_bonus;
		mem_lmt = mem_lmt + java_memory_bonus;
		// 从java0.policy文件中对代码的可执行权限进行授权
		if(lang==3){
			execute_cmd("/bin/cp %s/etc/java0.policy %s/java.policy", oj_home,work_dir);
            //改变文件的权限为当前目录的权限
			execute_cmd("chmod 755 %s/java.policy", work_dir);
			//将文件的所有者变为工作目录work_dir
			execute_cmd("chown judge %s/java.policy", work_dir);
		}
	}
	//限制时间内存大小上限
	if (time_lmt > 300 || time_lmt < 1)
		time_lmt = 300;
	if (mem_lmt > 1024 || mem_lmt < 1)
		mem_lmt = 1024;

	if (DEBUG)
		printf("time: %d mem: %d\n", time_lmt, mem_lmt);

	int Compile_OK;
    //根据语言编译程序，成功返回0
	Compile_OK = compile(lang,work_dir);
	 //编译失败，更新数据库solution表
	if (Compile_OK != 0) {
		addceinfo(solution_id);
		update_solution(solution_id, OJ_CE, 0, 0, 0, 0, 0.0);
		if(!turbo_mode)update_user(user_id);
		if(!turbo_mode)update_problem(p_id);
#ifdef _mysql_h
		if (!http_judge)
			mysql_close(conn);
#endif
		clean_workdir(work_dir);
		write_log("compile error");
		exit(0);
	} else {
	    //脚本语言更新solution表
		if(!turbo_mode)update_solution(solution_id, OJ_RI, 0, 0, 0, 0, 0.0);
		umount(work_dir);
	}
	//执行编译后的程序
	char fullpath[BUFFER_SIZE];
	char infile[BUFFER_SIZE];
	char outfile[BUFFER_SIZE];
	char userfile[BUFFER_SIZE];
	//将所有的目录下的测试文件全部读到data目录下
	sprintf(fullpath, "%s/data/%d", oj_home, p_id); // 数据目录


	DIR *dp;
	//结构体，获取文件目录内容
	dirent *dirp;
	// 用http远程获取测试数据文件
	if (p_id > 0 && http_judge && http_download)
		get_test_file(work_dir, p_id);

	//读取目录失败则子进程退出
	if (p_id > 0 && (dp = opendir(fullpath)) == NULL) {

		write_log("No such dir:%s!\n", fullpath);
#ifdef _mysql_h
		if (!http_judge)
			mysql_close(conn);
#endif
		exit(-1);
	}

	int ACflg, PEflg;
	ACflg = PEflg = OJ_AC;
	int namelen;
	int usedtime = 0, topmemory = 0;

	//create chroot for ruby bash python
	if (lang == 4)
		copy_ruby_runtime(work_dir);
	if (lang == 5)
		copy_bash_runtime(work_dir);
	if (lang == 6)
		copy_python_runtime(work_dir);
	if (lang == 7)
		copy_php_runtime(work_dir);
	if (lang == 8)
		copy_perl_runtime(work_dir);
	if (lang == 9)
		copy_mono_runtime(work_dir);
	if (lang == 10)
		copy_objc_runtime(work_dir);
	if (lang == 11)
		copy_freebasic_runtime(work_dir);
	if (lang == 12)
		copy_guile_runtime(work_dir);
	if (lang == 15)
		copy_lua_runtime(work_dir);
	if (lang == 16)
		copy_js_runtime(work_dir);


	double pass_rate = 0.0;//用于机算通过率
	int num_of_test = 0;
	int finalACflg = ACflg;
	if (p_id == 0) {  //custom input running
		printf("running a custom input...\n");
		get_custominput(solution_id, work_dir);

		 //根据语言设置进程资源的限制
		init_syscalls_limits(lang);
		//fork()产生子进程
		pid_t pidApp = fork();

		if (pidApp == 0) {
			run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
		} else {
			watch_solution(pidApp, infile, ACflg, isspj, userfile, outfile,
					solution_id, lang, topmemory, mem_lmt, usedtime, time_lmt,
					p_id, PEflg, work_dir);

		}
		if (finalACflg == OJ_TL) {
			usedtime = time_lmt * 1000;
		}
		if (ACflg == OJ_RE) {
			if (DEBUG)
				printf("add RE info of %d..... \n", solution_id);
			addreinfo(solution_id);
		} else {
			addcustomout(solution_id);
		}
		update_solution(solution_id, OJ_TR, usedtime, topmemory >> 10, 0, 0, 0);
		clean_workdir(work_dir);
		exit(0);
	}
	for (; (oi_mode || ACflg == OJ_AC|| ACflg == OJ_PE) && (dirp = readdir(dp)) != NULL;) {
       //检测是否是输入输出文件
		namelen = isInFile(dirp->d_name);
		if (namelen == 0)
			continue;

		if(http_judge&&http_download&&(!data_list_has(dirp->d_name))) 
			continue;
	
		prepare_files(dirp->d_name, namelen, infile, p_id, work_dir, outfile,
				userfile, runner_id);
		init_syscalls_limits(lang);

		pid_t pidApp = fork();

        //子进程直接判题，在/home/run0下生成user.out
		if (pidApp == 0) {
			run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
		} else {
			num_of_test++;
            //监视子进程是否执行完毕
			watch_solution(pidApp, infile, ACflg, isspj, userfile, outfile,
					solution_id, lang, topmemory, mem_lmt, usedtime, time_lmt,
					p_id, PEflg, work_dir);
            //进行结果的匹配判断
			judge_solution(ACflg, usedtime, time_lmt, isspj, p_id, infile,
					outfile, userfile, PEflg, lang, work_dir, topmemory,
					mem_lmt, solution_id, num_of_test);
			if (use_max_time) {
			 //当前程序运行时间设为max_case_time
				max_case_time =
						usedtime > max_case_time ? usedtime : max_case_time;
				usedtime = 0;
			}
		}
		//编译模式选择
		if (oi_mode) {
		   //
			if (ACflg == OJ_AC) {
				++pass_rate;
			}
			if (finalACflg < ACflg) {
				finalACflg = ACflg;
			}
			ACflg = OJ_AC;
		}
	}
	if (ACflg == OJ_AC && PEflg == OJ_PE)
		ACflg = OJ_PE;
	if (sim_enable && ACflg == OJ_AC && (!oi_mode || finalACflg == OJ_AC))
	{
		sim = get_sim(solution_id, lang, p_id, sim_s_id);
	} else {
		sim = 0;
	}

	if ((oi_mode && finalACflg == OJ_RE) || ACflg == OJ_RE) {
		if (DEBUG)
			printf("add RE info of %d..... \n", solution_id);
		addreinfo(solution_id);
	}
	if (use_max_time) {
		usedtime = max_case_time;
	}
	if (finalACflg == OJ_TL ) {
		usedtime = time_lmt * 1000;
		if (DEBUG)
                        printf("usedtime:%d\n",usedtime);
	}
	if (oi_mode) {
		if (num_of_test > 0)
		    //计算通过率
			pass_rate /= num_of_test;
			//更新数据到数据库
		update_solution(solution_id, finalACflg, usedtime, topmemory >> 10, sim,
				sim_s_id, pass_rate);
	} else {
		update_solution(solution_id, ACflg, usedtime, topmemory >> 10, sim,
				sim_s_id, 0);
	}
	if ((oi_mode && finalACflg == OJ_WA) || ACflg == OJ_WA) {
		if (DEBUG)
			printf("add diff info of %d..... \n", solution_id);
		if (!isspj)
			adddiffinfo(solution_id);
	}
	if(!turbo_mode)update_user(user_id);
	if(!turbo_mode)update_problem(p_id);
	clean_workdir(work_dir);

	if (DEBUG)
		write_log("result=%d", oi_mode ? finalACflg : ACflg);
#ifdef _mysql_h
	if (!http_judge)
		mysql_close(conn);
#endif
	if (record_call) {
		print_call_array();
	}
	closedir(dp);
	return 0;
}
