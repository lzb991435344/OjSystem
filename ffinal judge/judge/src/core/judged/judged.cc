
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <mysql/mysql.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/resource.h>

#define BUFFER_SIZE 1024
//lockfile的path
#define LOCKFILE "/var/run/judged.pid"
//文件具有的权限
//S_IRUSR 代表该文件所有者具有可读取的权限
//S_IWUSR 代表该文件所有者具有可写入的权限
//S_IRGRP 00040 权限，代表该文件用户组具有可读的权限
//S_IROTH 00004 权限，代表其他用户具有可读的权限
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define STD_MB 1048576

//定义判题的状态
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

//全局变量

static char lock_file[BUFFER_SIZE]=LOCKFILE;//全局的lockfile数组
static char host_name[BUFFER_SIZE];
static char user_name[BUFFER_SIZE];
static char password[BUFFER_SIZE];
static char db_name[BUFFER_SIZE];
static char oj_home[BUFFER_SIZE];
static char oj_lang_set[BUFFER_SIZE];//语言选项数组
static int port_number;
static int max_running;
static int sleep_time;
static int sleep_tmp;
static int oj_tot;
static int oj_mod;
static int http_judge = 0;
static char http_baseurl[BUFFER_SIZE];
static char http_username[BUFFER_SIZE];
static char http_password[BUFFER_SIZE];

static int  oj_redis = 0;
static char oj_redisserver[BUFFER_SIZE];
static int  oj_redisport;
static char oj_redisauth[BUFFER_SIZE];
static char oj_redisqname[BUFFER_SIZE];
static int turbo_mode = 0;


static bool STOP = false;
static int DEBUG = 0;
static int ONCE = 0;

//mysql数据库相关变量
#ifdef _mysql_h
static MYSQL *conn;//连接对象
static MYSQL_RES *res;//结果集
static MYSQL_ROW row;//行对象
static char query[BUFFER_SIZE];//查询得到结果的字符数组
#endif

void call_for_exit(int s) {
	STOP = true;//状态改变
	printf("Stopping judged...\n");
}

//写日志，格式化的常量指针
void write_log(const char *fmt, ...) {
	va_list ap;

	char buffer[4096];
	//格式化oj_home字符串写入buff
	sprintf(buffer, "%s/log/client.log", oj_home);

	FILE *fp = fopen(buffer, "ae+");
	if (fp == NULL) {
		fprintf(stderr, "openfile error!\n");
		//获取当前系统的用户所在目录
		system("pwd");
	}

    //变参处理
	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	fprintf(fp, "%s\n", buffer);
	if (DEBUG)
		printf("%s\n", buffer);
	va_end(ap);

	fclose(fp);
}

//字符串匹配后要去除字符串中的多余字符
int after_equal(char * c) {
	int i = 0;
	//去除'='号和字符串结尾字符
	for (; c[i] != '\0' && c[i] != '='; i++)
		;
	return ++i;
}

//返回字符串副本
void trim(char * c) {
	char buf[BUFFER_SIZE];
	char * start, *end;
	//char *strcpy(char* dest, const char *src)
	//把从src地址开始且含有NULL结束符的字符串复制到以dest开始的地址空间
	strcpy(buf, c);
	start = buf;//指针指向字符数组的首地址
	while (isspace(*start))
		start++;
	end = start;
	while (!isspace(*end))
		end++;
	//加'\0'
	*end = '\0';
	//复制字符串
	strcpy(c, start);
}
//读取缓存,主要是字符串的读取
bool read_buf(char * buf, const char * key, char * value) {
	//和配置文件中的字符串进行比较
	if (strncmp(buf, key, strlen(key)) == 0) {
		//赋值到字符数组value
		strcpy(value, buf + after_equal(buf));

		trim(value);//去除空格,返回字符串的副本

		//debug模式下打印值
		if (DEBUG)
			printf("%s\n", value);
		return 1;
	}
	return 0;
}

//读取整型数据
void read_int(char * buf, const char * key, int * value) {
	char buf2[BUFFER_SIZE];
	if (read_buf(buf, key, buf2))
		sscanf(buf2, "%d", value);

}
// 初始化数据库配置信息
void init_mysql_conf() {
	FILE *fp = NULL;
	char buf[BUFFER_SIZE];
	host_name[0] = 0;
	user_name[0] = 0;
	password[0] = 0;
	db_name[0] = 0;
	port_number = 3306;
	max_running = 3;
	sleep_time = 1;
	oj_tot = 1;
	oj_mod = 0;
	//语言选择选项
	strcpy(oj_lang_set, "0,1,2,3");
	//打开配置文件,根据指针操作数据
	fp = fopen("./etc/judge.conf", "r");
	if (fp != NULL) {

		/**
		char *fgets(char *buf, int bufsize, FILE *stream);
		参数1:指向存储数据的地址
		参数2:数据的大小
		参数3:文件流的指针
		返回值:成功则返回第一个参数buf;到文件尾,则eof指示器被设置，
		如果还没读入任何字符就遇到这种情况，则buf保持原来的内容，
		返回NULL；如果发生读入错误，error指示器被设置，返回NULL，
		buf的值可能被改变。
		*/
		while (fgets(buf, BUFFER_SIZE - 1, fp)) {

			read_buf(buf, "OJ_HOST_NAME", host_name);
			read_buf(buf, "OJ_USER_NAME", user_name);
			read_buf(buf, "OJ_PASSWORD", password);
			read_buf(buf, "OJ_DB_NAME", db_name);
			read_int(buf, "OJ_PORT_NUMBER", &port_number);
			read_int(buf, "OJ_RUNNING", &max_running);
			read_int(buf, "OJ_SLEEP_TIME", &sleep_time);
			read_int(buf, "OJ_TOTAL", &oj_tot);

			read_int(buf, "OJ_MOD", &oj_mod);

			read_int(buf, "OJ_HTTP_JUDGE", &http_judge);
			read_buf(buf, "OJ_HTTP_BASEURL", http_baseurl);
			read_buf(buf, "OJ_HTTP_USERNAME", http_username);
			read_buf(buf, "OJ_HTTP_PASSWORD", http_password);
			read_buf(buf, "OJ_LANG_SET", oj_lang_set);
			
			read_int(buf, "OJ_REDISENABLE", &oj_redis);
                        read_buf(buf, "OJ_REDISSERVER", oj_redisserver);
                        read_int(buf, "OJ_REDISPORT", &oj_redisport);
                        read_buf(buf, "OJ_REDISAUTH", oj_redisauth);
                        read_buf(buf, "OJ_REDISQNAME", oj_redisqname);
                        read_int(buf, "OJ_TURBO_MODE", &turbo_mode);
		}
#ifdef _mysql_h
		sprintf(query,
				"SELECT solution_id FROM solution WHERE language in (%s) and result<2 and MOD(solution_id,%d)=%d ORDER BY result ASC,solution_id ASC limit %d",
				oj_lang_set, oj_tot, oj_mod, max_running * 2);
#endif
		sleep_tmp = sleep_time;
	}
}
//设置进程资源限制
void run_client(int runid, int clientid) {
	char buf[BUFFER_SIZE], runidstr[BUFFER_SIZE];
	struct rlimit LIM;
	LIM.rlim_max = 800;
	LIM.rlim_cur = 800;
	/*
	int getrlimit( int resource, struct rlimit *rlptr );
    int setrlimit( int resource, const struct rlimit *rlptr );
    两个函数返回值：若成功则返回0，若出错则返回非0值
	*/
	setrlimit(RLIMIT_CPU, &LIM); //cpu时间限制

	LIM.rlim_max = 180 * STD_MB;
	LIM.rlim_cur = 180 * STD_MB;
	setrlimit(RLIMIT_FSIZE, &LIM);//文件大小限制

	LIM.rlim_max = STD_MB << 11;
	LIM.rlim_cur = STD_MB << 11;
	setrlimit(RLIMIT_AS, &LIM);//虚拟内存大小

	LIM.rlim_cur = LIM.rlim_max = 400;
	setrlimit(RLIMIT_NPROC, &LIM); //每个实际用户id拥有的最大进程数

	//格式化字符串,写进字符数组
	sprintf(runidstr, "%d", runid);
	sprintf(buf, "%d", clientid);


	if (!DEBUG)
		/**
		int execl(const char *path, const char *arg, ...);
		参数1:路径,const
		参数2:字符参数
		返回值:成功则不返回值， 失败返回-1， 失败原因存于errno中，
		可通过perror()打印
		*/
	  //执行judge_client程序,非debug模式
		execl("/usr/bin/judge_client", "/usr/bin/judge_client", runidstr, buf,
				oj_home, (char *) NULL);
	else
	     //开启DEBUG,该模式下添加一个参数"debug"
		execl("/usr/bin/judge_client", "/usr/bin/judge_client", runidstr, buf,
				oj_home, "debug", (char *) NULL);
}
#ifdef _mysql_h
int executesql(const char * sql) {
    //执行由query指向的SQL查询，包含二进制的查询，速度可提升
	if (mysql_real_query(conn, sql, strlen(sql))) {
		//debug模式下
		if (DEBUG)
			//打印log表示查询不成功
			write_log("%s", mysql_error(conn));
		sleep(20);//sleep 20s
		conn = NULL;//将指针赋值为空
		return 1;
	} else//查询成功直接返回
		return 0;
}
#endif

#ifdef _mysql_h
int init_mysql() 
{
	if (conn == NULL) 
	{
		//初始化
		conn = mysql_init(NULL);
		//连接数据库
		const char timeout = 30;

		//设置连接参数影响连接行为，以秒为单位
		mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

		//连接数据库引擎
		if (!mysql_real_connect(conn, host_name, user_name, password, db_name,
				port_number, 0, 0)) {
			if (DEBUG)//debug模式下打印错误信息
				write_log("%s", mysql_error(conn));
			sleep(2);
			return 1;
		} else {
			//成功连接数据库引擎
			return 0;
		}
	} else {
		/**
		execute(sql)
	(1)ResultSet executeQuery(String sql); 执行SQL查询，并返回ResultSet 对象。
	(2)int executeUpdate(String sql); 可执行增，删，改，返回执行受到影响的行数。
	(3)boolean execute(String sql);	
		*/
		//设置编码格式
		return executesql("set names utf8");
	}
}
#endif
FILE * read_cmd_output(const char * fmt, ...) {
	char cmd[BUFFER_SIZE];

	FILE * ret = NULL;
	/**
（1）首先在函数里定义一具VA_LIST型的变量，这个变量是指向参数的指针；
（2）然后用VA_START宏初始化刚定义的VA_LIST变量；
（3）然后用VA_ARG返回可变的参数，VA_ARG的第二个参数是你要返回的参数的类型
（如果函数有多个可变参数的，依次调用VA_ARG获取各个参数）；
（4）最后用VA_END宏结束可变参数的获取。
	
	*/
	va_list ap;

	va_start(ap, fmt);

	/**
	函数名: vsprintf
   返回值: 正常情况下返回生成字串的长度(除去\0),错误情况返回负值
   用 法: int vsprintf(char *string, char *format, va_list param);
   //将param 按格式format写入字符串string中
   注: 该函数会出现内存溢出情况,建议使用vsnprintf
	*/
	vsprintf(cmd, fmt, ap);
	va_end(ap);
	//调用fork()产生子进程，子进程(pid == 0)中调用/bin/sh -c来执行参数cmd的指令
	ret = popen(cmd, "r");
	return ret;
}
int read_int_http(FILE * f) {
	char buf[BUFFER_SIZE];
	//从文件流中读取n个数据到buf字符数组
	fgets(buf, BUFFER_SIZE - 1, f);
	return atoi(buf);//将字符数组转换成整型
}
//检验是否已经登录
bool check_login() {
	//执行的cmd
	const char * cmd =
			"wget --post-data=\"checklogin=1\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	int ret = 0;

	//执行cmd返回的结果地址,使用文件指针变量指向该地址
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);

	//文件流中读取字符数组转换成int类型
	ret = read_int_http(fjobs);
	//关闭文件流
	pclose(fjobs);
	
	//返回时判断返回结果是否大于0
	return ret > 0;
}
//登录账号
void login() {
	if (!check_login()) {
		char cmd[BUFFER_SIZE];
		sprintf(cmd,
				"wget --post-data=\"user_id=%s&password=%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/login.php\"",
				http_username, http_password, http_baseurl);
		/**
		实际上system()函数执行了三步操作：
        1.fork一个子进程；
        2.在子进程中调用exec函数去执行command；
        3.在父进程中调用wait去等待子进程结束。
        对于fork失败，system()函数返回-1。
        如果exec执行成功，也即command顺利执行完毕，
		则返回command通过exit或return返回的值。
		
			
      int system(const char * cmdstring)
       {
	    pid_t pid;
	    int status;

        if(cmdstring == NULL)
         {
	       return (1); //如果cmdstring为空，返回非零值，一般为1
          }

         if((pid = fork())<0)
         {
	       status = -1; //fork失败，返回-1
          }
         else if(pid == 0)//子进程执行bash命令,参数值是传入的const的命令行
         {
	       execl("/bin/sh", "sh", "-c", cmdstring, (char *)0);
	       _exit(127); // exec执行失败返回127，注意exec只在失败时才返回现在的进程，成功的话现在的进程就不存在啦~~
          }
         else //父进程
        {
	     while(waitpid(pid, &status, 0) < 0)
	       {
		      if(errno != EINTR)
		        {
			        status = -1; //如果waitpid被信号中断，则返回-1
		         	break;
		         }
	         }
          }
	        return status; //如果waitpid成功，则返回子进程的返回状态
          }
*/	
		system(cmd);
	}
}
//更新判题数量
int _get_jobs_http(int * jobs) {
	login();
	int ret = 0;
	int i = 0;
	char buf[BUFFER_SIZE];
	const char * cmd =
			"wget --post-data=\"getpending=1&oj_lang_set=%s&max_running=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, oj_lang_set, max_running, http_baseurl);
	
	while (fscanf(fjobs, "%s", buf) != EOF) {
	//将buf中数据转换成整型
		int sid = atoi(buf);
		if (sid > 0)
			//把题目的id号放进一个数组
			jobs[i++] = sid;
	}
	//关闭文件指针
	pclose(fjobs);
	ret = i;
	 //小于等于进程最大数量时，将题目放进jobs数组
	while (i <= max_running * 2)
		jobs[i++] = 0;
	return ret;
}
#ifdef _mysql_h
//取得待评测题目信息到jobs数组
int _get_jobs_mysql(int * jobs) {
	if (mysql_real_query(conn, query, strlen(query))) {
		if (DEBUG)
			write_log("%s", mysql_error(conn));
		sleep(20);
		return 0;
	}
	//返回多个结果集
	res = mysql_store_result(conn);

	int i = 0;
	int ret = 0;
	/*
	array mysql_fetch_row(int result)
	result：由函数mysql_query()或mysql_db_query()返回的结果标识，用来指定所要获取的数据的SQL语句类型。
函数返回值如下。
   成功：一个数组，该数组包含了查询结果集中当前行数据信息，数组下标范围0～记录属性数−1，数组中的第i个元素值为该记录第i个属性上的值。
   失败：false。
	*/
	while (res!=NULL && (row = mysql_fetch_row(res)) != NULL) {
	    //字符转为整型数
		jobs[i++] = atoi(row[0]);
	}
	//执行一个mysql的executesql()函数进行判断当前
	if(res!=NULL && !executesql("commit")){
		mysql_free_result(res);//释放内存
		res=NULL;
	}
	else i=0;
	ret = i;
	 //设置最大工作数目
	while (i <= max_running * 2)
		jobs[i++] = 0;
	return ret;
}
#endif
/**int _get_jobs_redis(int * jobs){
        int ret=0;
        const char * cmd="redis-cli -h %s -p %d -a %s --raw rpop %s";
        while(ret<=max_running){
                FILE * fjobs = read_cmd_output(cmd,oj_redisserver,oj_redisport,oj_redisauth,oj_redisqname);
                if(fscanf(fjobs,"%d",&jobs[ret])==1){
                        ret++;
                        pclose(fjobs);
                }else{
                        pclose(fjobs);
                        break;
                }
        }
        int i=ret;
        while (i <= max_running * 2)
                jobs[i++] = 0;
        if(DEBUG) printf("redis return %d jobs",ret);
        return ret;
}*/

//根据参数调函数返回判题数量
int get_jobs(int * jobs) {
	if (http_judge) {
		return _get_jobs_http(jobs);
	} else {		
		if(oj_redis){
                        return _get_jobs_redis(jobs);
                }else{
#ifdef _mysql_h

                        return _get_jobs_mysql(jobs);
#else
			return 0;
#endif
                }
	}
}

#ifdef _mysql_h
//数据库模式更新初始化solution表
bool _check_out_mysql(int solution_id, int result) {
	char sql[BUFFER_SIZE];
	//使用limit 1 进行更新的数量的限制
	//使用sprintf()格式化sql语句
	sprintf(sql,
			"UPDATE solution SET result=%d,time=0,memory=0,judgetime=NOW() WHERE solution_id=%d and result<2 LIMIT 1",
			result, solution_id);
	if (mysql_real_query(conn, sql, strlen(sql))) {

		//打印日志文件
		syslog(LOG_ERR | LOG_DAEMON, "%s", mysql_error(conn));
		return false;
	} else {
		// mysql_affected_rows(conn) 
	   //影响行数，影响行数大于0表示更新成功
		if (conn != NULL && mysql_affected_rows(conn) > 0ul)
			return true;
		else
			return false;
	}
}
#endif

//http模式更新表
bool _check_out_http(int solution_id, int result) {
	login();
	//需要执行的命令
	const char * cmd =
			"wget --post-data=\"checkout=1&sid=%d&result=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	int ret = 0;
	FILE * fjobs = read_cmd_output(cmd, solution_id, result, http_baseurl);
	fscanf(fjobs, "%d", &ret);
	pclose(fjobs);
	return ret;
}

//更新solution表，根据参数选择执行函数
bool check_out(int solution_id, int result) {
        if(oj_redis||oj_tot>1) return true;
	if (http_judge) {
	 //web插入solution表，core更新solution result,web轮询solution result
		return _check_out_http(solution_id, result);
	} else{
#ifdef _mysql_h
		return _check_out_mysql(solution_id, result);
#else
		return 0;
#endif
	}
}
//开始执行
int work() {
	static int retcnt = 0; //完成评测题目的数量
	int i = 0;
	//静态的数组
	static pid_t ID[100]; //保存当前的子进程ID
	static int workcnt = 0; //目前使用judge_client进程的数
	int runid = 0;
	//jobs数组开max_running的两倍
	int jobs[max_running * 2 + 1];
	pid_t tmp_pid = 0;//当前的tmp_pid设置为0

	//从数据库获取判题任务
	if (!get_jobs(jobs)){
		return 0;
	}
	//执行提交
	for (int j = 0; jobs[j] > 0; j++) {
		runid = jobs[j];
		if (runid % oj_tot != oj_mod)
			continue;
		//debug模式下打印出当前的运行题目的id
		if (DEBUG)
			write_log("Judging solution %d", runid);

		//当前使用的客户端的进程数量大于等于最大允许的数量
		if (workcnt >= max_running)
		{
			//pid_t waitpid(pid_t pid, int * status, int options);
			//参数1:参数 pid 为欲等待的子进程识别码,
			/*
			 pid<-1 等待进程组识别码为 pid 绝对值的任何子进程。
             pid=-1 等待任何子进程,相当于 wait()。
             pid=0 等待进程组识别码与目前进程相同的任何子进程。
             pid>0 等待任何子进程识别码为 pid 的子进程。
			*/
			//参数2:参数 status 可以设成 NULL。
			//参数3:
			/*
			参数options提供了一些额外的选项来控制waitpid，参数option 可以为 0 或可以用"|"运算符把它们连接起来使用，比如：
            ret=waitpid(-1,NULL,WNOHANG | WUNTRACED);
            如果我们不想使用它们，也可以把options设为0，
			*/
			//返回值:成功返回子进程的识别码,否则-1
			tmp_pid = waitpid(-1, NULL, 0);     // 等子进程结束
			//该循环的作用是匹配静态子进程数组中的进程id号
			for (i = 0; i < max_running; i++)
			{     // 获取子进程ID
				if (ID[i] == tmp_pid)//当前子进程在进行判题
				{
					workcnt--;//当前的判题数量减1
					retcnt++;//执行结果加1
					ID[i] = 0;//设置该子进程的id号为0
					break;//退出循环
				}
			}
		}
		else
		{     //已清除运行完毕的client
            // 该循环用于查找判题进程ID
			for (i = 0; i < max_running; i++)    
				if (ID[i] == 0)//无效的进程id
					break;//直接退出循环
		}
		if(i<max_running){
			if (workcnt < max_running && check_out(runid, OJ_CI)) {
				workcnt++;
				//创建子进程
				ID[i] = fork();
				//子进程的pid==0
				if (ID[i] == 0) { //子进程在执行代码
					if (DEBUG)
						write_log("<<=sid=%d===clientid=%d==>>\n", runid, i);
					run_client(runid, i);
					exit(0); //子进程退出
				}
			} else {
				ID[i] = 0;
			}
		}
	}
	//父进程执到这里时查看当前的进程中是否有子僵尸进程，有就回收
	while ((tmp_pid = waitpid(-1, NULL, 0)) > 0) {
		for (i = 0; i < max_running; i++){
		   //获取子进程ID
			if (ID[i] == tmp_pid){
				workcnt--;
				retcnt++;
				ID[i] = 0;
				break;
			}
		}
		printf("tmp_pid = %d\n", tmp_pid);
	}
	if (!http_judge) {
#ifdef _mysql_h
		if(res!=NULL) {
			mysql_free_result(res);  //释放内存
			res=NULL;
		}
		executesql("commit");
#endif
	}
	if (DEBUG && retcnt)
		write_log("<<%ddone!>>", retcnt);
	return retcnt;
}
//对文件加锁，多进程读取文件要锁住文件之后进行写数据
int lockfile(int fd) {
	/**
	struct flock
	{
	short l_type;//F_RDLCK, F_WRLCK, or F_UNLCK
	off_t l_start;//相对于l_whence的偏移值，字节为单位
	short l_whence;//从哪里开始：SEEK_SET, SEEK_CUR, or SEEK_END
	off_t l_len;//长度, 字节为单位; 0 意味着缩到文件结尾
	pid_t l_pid;//returned with F_GETLK
};
*/
	struct flock fl;
	fl.l_type = F_WRLCK;//设置类型为写互斥锁
	fl.l_start = 0;   //相对于l_whence偏移量
	fl.l_whence = SEEK_SET;//开始位置
	fl.l_len = 0;  //长度
	//fcntl改变文件描述符的状态
	//锁住文件描述符fd,第二个参数为设置锁，第三个为指向一个flock的结构体指针
	//返回值和命令有关,出错则返回1
	return (fcntl(fd, F_SETLK, &fl));
}

//检测进程是否已经在运行
int already_running() {
	int fd;
	char buf[16];
	//O_RDWR 以可读写方式打开文件
	// O_CREAT 若欲打开的文件不存在则自动建立该文件
	//返回值0或者-1,-1为权限被限制
	fd = open(lock_file, O_RDWR | O_CREAT, LOCKMODE);
    //权限错误写入系统日志
	if (fd < 0) {
		syslog(LOG_ERR | LOG_DAEMON, "can't open %s: %s", LOCKFILE,
				strerror(errno));
		exit(1);
	}
	if (lockfile(fd) < 0) {
	   //权限不足或者再试一次
		if (errno == EACCES || errno == EAGAIN) {
			close(fd);
			return 1;
		}
		syslog(LOG_ERR | LOG_DAEMON, "can't lock %s: %s", LOCKFILE,
				strerror(errno));
		exit(1);
	}
	//将lockfile文件大小直接改为0
	ftruncate(fd, 0);

	//格式化进程pid写入buf中
	sprintf(buf, "%d", getpid());

	//重新将缓存中的数据写到lockfile中
	write(fd, buf, strlen(buf) + 1);
	return (0);
}

//开启守护进程，用于监控前端用户提交的solutionid
int daemon_init(void)
{
	pid_t pid;
	//fork失败
	if ((pid = fork()) < 0)
		return (-1);
   //退出父进程
	else if (pid != 0)
		exit(0);

	//子进程继续执行 fork()之后pid==0
	setsid(); //成为会话组的组长
	chdir(oj_home); //改变工作的目录
	umask(0); //赋予文件最高权限
    //关闭标准流,标准输入,标准输出,标准错误流
	close(0);
	close(1);
	close(2);

	//将文件丢入blackhole，丢弃标准流
	int fd = open( "/dev/null", O_RDWR );
	//复制文件描述符，标准输入，标准输出，标准错误流
	dup2( fd, 0 );
	dup2( fd, 1 );
	dup2( fd, 2 );
	//文件描述符不正确 
	if ( fd > 2 ){
		close( fd );
	}
	return (0);
}
void turbo_mode2(){
#ifdef _mysql_h
	if(turbo_mode==2){
			char sql[BUFFER_SIZE];//字符数组作为缓冲区
			//格式化sql语句
			sprintf(sql," CALL `sync_result`();");
			//int mysql_real_query(MYSQL *mysql, const char *query, unsigned int length)
			//对二进制数据的查询,比mysql_query()更快
			//参数1:指向sql的地址,
			//参数2:常量字符串,通常是sql语句
			//参数3:sql的长度
			if (mysql_real_query(conn, sql, strlen(sql)));
	}
#endif
}
int main(int argc, char** argv) {
	DEBUG = (argc > 2); //参数大于2开启DEBUG
	ONCE = (argc > 3);
	if (argc > 1)
		strcpy(oj_home, argv[1]);
	else//argc <= 1时的的工作目录为/home/judge
		strcpy(oj_home, "/home/judge");

	//改变工作的目录
	chdir(oj_home);

    //将buffer(进程号)写入lockfile,先格式化字符串再写入文件
	sprintf(lock_file,"%s/etc/judge.pid",oj_home);
	if (!DEBUG)//非debug模式
		daemon_init();//守护进程初始化

	//守护进程已经在运行
	if ( already_running()) {
		//打log日志,打印出当前的工作目录
		syslog(LOG_ERR | LOG_DAEMON,
				"This daemon program is already running!\n");
		printf("%s already has one judged on it!\n",oj_home);
		return 1;
	}
	if(!DEBUG)
		system("/sbin/iptables -A OUTPUT -m owner --uid-owner judge -j DROP");
//初始化数据库
#ifdef _mysql_h
	init_mysql_conf();
#endif
    //指定信号到达即执行相应函数
	//参数1:宏
	//参数2:执行的回调函数
	signal(SIGQUIT, call_for_exit);
	signal(SIGKILL, call_for_exit);
	signal(SIGTERM, call_for_exit);

	int j = 1;
	int n = 0;
	while (1) {
		while (j && (http_judge
#ifdef _mysql_h
			 || !init_mysql()
#endif
		)) {
			j = work();
			n += j;
			if(turbo_mode == 2 && (n > max_running*10||j < max_running)){
				turbo_mode2();
				n=0;
			}
			if(ONCE) break;
		}
		turbo_mode2();
		//非debug模式
		if(ONCE) break;
		//无评判任务，系统休息，暂时释放系统资源，避免守护进程死循环占用资源
		sleep(sleep_time);
		j = 1;
	}
	return 0;
}
