#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>

#define DATABASE "my.db"
#define LEN 16
#define DATALEN 128

#define MANAGER_LOGIN 	1
#define USER_LOGIN 		2
#define MANAGER_Q1 		11
#define MANAGER_Q2 		12
#define MANAGER_M 		2
#define MANAGER_A 		3
#define MANAGER_D 		4
#define MANAGER_H 		5
#define MANAGER_QUIT 	6
#define USER_Q 			1
#define USER_M 			2
#define USER_QUIT 		3

typedef struct{
	int id;
	int type;
	char name[LEN];
	char password[8];
	int age;
	char phone[LEN];
	char addr[DATALEN];
	char position[DATALEN];
	char date[DATALEN];
	int level;
	double salaray;
}staff_info_t;

typedef struct {
	int msgtype;
	int usertype;
	char username[LEN];
	char password[8];
	char msg[DATALEN];
	int flags;
	staff_info_t info;
}MSG;

sqlite3 *db;

int do_client(int fd)
{
	MSG msg;
	memset(&msg,0,sizeof(MSG));
	while(recv(fd,&msg,sizeof(MSG),0) > 0)
	{
		switch(msg.usertype)
		{
			case MANAGER_LOGIN:manager_login(&msg,fd);break;
			case USER_LOGIN:user_login(&msg,fd);break;
		}
	}
	exit(0);
}

int manager_login(MSG *msg,int fd)
{
	char *errmsg;
	int m,n;
	char sql[128] = {0};
	char **resultp;

	sprintf(sql,"select * from user_info where type = 0 and name = '%s' and password = '%s';",\
			msg->username,msg->password);
	printf("sql = %s\n",sql);
	if(sqlite3_get_table(db,sql,&resultp,&m,&n,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	if(m == 1)
	{
		strcpy(msg->msg,"OK");
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("fail to send ok in manager_login");
			return -1;
		}
		
		manager_mode(msg,fd);

		return 1;
	}
	if(m == 0)
	{
		strcpy(msg->msg,"usr/password/type wrong.");
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("fail to send wrong manager_login");
			return -1;
		}
	}
	
	return 0;
}

int manager_mode(MSG * msg,int fd)
{
	while(recv(fd,msg,sizeof(MSG),0) > 0)
	{
		switch(msg->msgtype)
		{
			case MANAGER_Q1:manager_query_name(msg,fd);break;
			case MANAGER_Q2:manager_query_all(msg,fd);break;
			case MANAGER_M:manager_modify(msg,fd);break;
			case MANAGER_A:manager_add(msg,fd);break;
			case MANAGER_D:manager_delete(msg,fd);break;
			case MANAGER_H:manager_history(msg,fd);break;
			case MANAGER_QUIT:return 0;break;
		}
	}
}

int manager_query_name(MSG * msg,int fd)
{
	char *errmsg;
	int m,n;
	char sql[128] = {0};
	char **resultp;
	
	sprintf(sql,"select * from user_info where name = '%s';",\
			msg->msg);
	printf("sql = %s\n",sql);
	if(sqlite3_get_table(db,sql,&resultp,&m,&n,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	memset(msg->msg,0,sizeof(char) * DATALEN);
	if(m > 0)
	{
		memset(&msg->info,0,sizeof(staff_info_t));
		strcpy(msg->msg,"OK");
		int i,j,count = n;
		printf("m = %d,n = %d\n",m,n);
		for(i = 0;i < m;i++)
		{
			for(j = 0;j < n;j++)
			{
				printf("%d:%s\n",__LINE__,resultp[count]);
				if(j == 0) msg->info.id = atoi(resultp[count++]);
				if(j == 1) msg->info.type = atoi(resultp[count++]);
				if(j == 2) strcpy(msg->info.name,resultp[count++]);
				if(j == 3) strcpy(msg->info.password,resultp[count++]);
				if(j == 4) msg->info.age = atoi(resultp[count++]);
				if(j == 5) strcpy(msg->info.phone,resultp[count++]);
				if(j == 6) strcpy(msg->info.addr,resultp[count++]);
				if(j == 7) strcpy(msg->info.position,resultp[count++]);
				if(j == 8) strcpy(msg->info.date,resultp[count++]);
				if(j == 9) msg->info.level = atoi(resultp[count++]);
				if(j == 10) msg->info.salaray = atoll(resultp[count++]);
			}
		}
		printf("id = %d\n",msg->info.id);
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("query_name send error");
			return -1;
		}
	}
	if(m == 0)
	{
		strcpy(msg->msg,"No member");
		send(fd,msg,sizeof(MSG),0);
	}
	return 0;
}

int manager_query_all(MSG * msg,int fd)
{
	char *errmsg;
	int m,n;
	char sql[128] = {0};
	char **resultp;
	
	sprintf(sql,"select * from user_info;");
	printf("sql = %s\n",sql);
	if(sqlite3_get_table(db,sql,&resultp,&m,&n,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	if(m > 0)
	{
		memset(msg->msg,0,sizeof(char) * DATALEN);
		memset(&msg->info,0,sizeof(staff_info_t));
		strcpy(msg->msg,"OK");
		int i,j,count = n;
		printf("m = %d,n = %d\n",m,n);
		msg->flags = m;
		for(i = 0;i < m;i++)
		{
			for(j = 0;j < n;j++)
			{
				printf("%d:%s\n",__LINE__,resultp[count]);
				if(j == 0) msg->info.id = atoi(resultp[count++]);
				if(j == 1) msg->info.type = atoi(resultp[count++]);
				if(j == 2) strcpy(msg->info.name,resultp[count++]);
				if(j == 3) strcpy(msg->info.password,resultp[count++]);
				if(j == 4) msg->info.age = atoi(resultp[count++]);
				if(j == 5) strcpy(msg->info.phone,resultp[count++]);
				if(j == 6) strcpy(msg->info.addr,resultp[count++]);
				if(j == 7) strcpy(msg->info.position,resultp[count++]);
				if(j == 8) strcpy(msg->info.date,resultp[count++]);
				if(j == 9) msg->info.level = atoi(resultp[count++]);
				if(j == 10) msg->info.salaray = atoll(resultp[count++]);
			}
			if(send(fd,msg,sizeof(MSG),0) < 0)
			{
				perror("query_name send error");
				return -1;
			}
		}
	}

	return 0;
}

int manager_modify(MSG * msg,int fd)
{
	char * errmsg;
	char sql[128];
	char sqlh[128];
	time_t t;
	struct tm *tp;
	
	time(&t);
	tp = localtime(&t);
	switch(msg->flags)
	{
		case 1:sprintf(sql, "update user_info set name='%s' where id=%d;",\
						msg->info.name, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的姓名为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.name);
			   break;
		case 2:sprintf(sql, "update user_info set password='%s' where id=%d;",\
					   msg->info.password, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的密码为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.password);
			   break;
		case 3:sprintf(sql, "update user_info set age=%d where id=%d;",\
					   msg->info.age, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的年龄为%d');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.age);
			   break;
		case 4:sprintf(sql, "update user_info set phone='%s' where id=%d;",\
					   msg->info.phone, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的电话为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.phone);
			   break;
		case 5:sprintf(sql, "update user_info set address='%s' where id=%d;",\
					   msg->info.addr, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的地址为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.addr);
			   break;
		case 6:sprintf(sql, "update user_info set position='%s' where id=%d;",\
					   msg->info.position, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的职位为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.position);
			   break;
		case 7:sprintf(sql, "update user_info set date='%s' where id=%d;",\
					   msg->info.date, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的入职年月为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.date);
			   break;
		case 8:sprintf(sql, "update user_info set level=%d where id=%d;",\
					   msg->info.level, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的等级为%d');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.level);
			   break;
		case 9:sprintf(sql, "update user_info set =%lf where id=%d;",\
					   msg->info.salaray, atoi(msg->msg));
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%d的工资为%lf');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,atoi(msg->msg),msg->info.salaray);
			   break;
	}
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	if(sqlite3_exec(db,sqlh, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}

	memset(msg->msg,0,sizeof(msg->msg));
	strcpy(msg->msg,"OK");
	if(send(fd,msg,sizeof(MSG),0) < 0)
	{
		perror("manager_modify send error");
		return -1;
	}
	return 0;
}

int manager_add(MSG *msg,int fd)
{
	char * errmsg;
	char sql[128];
	char sqlh[128];
	time_t t;
	struct tm *tp;
	
	time(&t);
	tp = localtime(&t);
	sprintf(sql, "insert into user_info values(%d,%d,'%s','%s',%d,'%s','%s','%s','%s',%d,%lf);",\
			msg->info.id,msg->info.type,msg->info.name,msg->info.password,msg->info.age,msg->info.phone,\
			msg->info.addr,msg->info.position,msg->info.date,msg->info.level,msg->info.salaray);
	printf("%s\n", sql);
	sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s添加了%s用户');",\
		   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
		   msg->username,msg->info.name);
	
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	if(sqlite3_exec(db,sqlh, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	strcpy(msg->msg,"OK");
	if(send(fd,msg,sizeof(MSG),0) < 0)
	{
		perror("add recv error");
		return -1;
	}

	return 0;
}

int manager_delete(MSG *msg,int fd)
{
	char * errmsg;
	char sql[128];
	char sqlh[128];
	time_t t;
	struct tm *tp;
	
	time(&t);
	tp = localtime(&t);
	sprintf(sql, "delete from user_info where id=%d and name='%s';",msg->info.id,msg->info.name);
	printf("%s\n", sql);
	sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s删除了%s用户');",\
		   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
		   msg->username,msg->info.name);
	
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	printf("%s\n",sql);
	if(sqlite3_exec(db,sqlh, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n", errmsg);
		return -1;
	}
	memset(msg->msg,0,sizeof(msg->msg));
	strcpy(msg->msg,"OK");
	if(send(fd,msg,sizeof(MSG),0) < 0)
	{
		perror("add recv error");
		return -1;
	}
	return 0;
}


int manager_history(MSG *msg,int fd)
{
	char *errmsg = NULL;
	char **resultp = NULL;  //指针数组
	int nrow, ncolumn;
	if (0 != sqlite3_get_table(db, "select * from history;", &resultp, &nrow, \
				&ncolumn, &errmsg))
	{
		fprintf(stderr, "getsourcefilter table: %s\n", errmsg);
		return -1;
	}
	int i, count = ncolumn;
	msg->flags = nrow;
	for (i = 0; i < nrow; i++)
	{
		memset(msg->msg,0,sizeof(msg->msg));
		strcpy(msg->msg,resultp[count++]);
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("history error");
			return -1;
		}
	}
	return 0;
}

int user_login(MSG *msg,int fd)
{
	char *errmsg;
	int m,n;
	char sql[128] = {0};
	char **resultp;

	sprintf(sql,"select * from user_info where type = 1 and name = '%s' and password = '%s';",\
			msg->username,msg->password);
	printf("sql = %s\n",sql);
	if(sqlite3_get_table(db,sql,&resultp,&m,&n,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	if(m == 1)
	{
		strcpy(msg->msg,"OK");
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("fail to send ok in manager_login");
			return -1;
		}
		
		user_mode(msg,fd);

		return 1;
	}
	if(m == 0)
	{
		strcpy(msg->msg,"usr/password/type wrong.");
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("fail to send wrong manager_login");
			return -1;
		}
	}
	
	return 0;
}

int user_mode(MSG *msg,int fd)
{
	while(recv(fd,msg,sizeof(MSG),0) > 0)
	{
		switch(msg->msgtype)
		{
			case USER_Q:user_query(msg,fd);break;
			case USER_M:user_modify(msg,fd);break;
			case USER_QUIT:return 0;break;
		}
	}
	return 0;
}

int user_query(MSG *msg,int fd)
{
	char *errmsg;
	int m,n;
	char sql[128] = {0};
	char **resultp;
	
	sprintf(sql,"select * from user_info where name = '%s';",\
			msg->username);
	printf("sql = %s\n",sql);
	if(sqlite3_get_table(db,sql,&resultp,&m,&n,&errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	memset(msg->msg,0,sizeof(msg->msg));
	if(m == 1)
	{
		memset(&msg->info,0,sizeof(staff_info_t));
		strcpy(msg->msg,"OK");
		int i,j,count = n;
		printf("m = %d,n = %d\n",m,n);
		for(i = 0;i < m;i++)
		{
			for(j = 0;j < n;j++)
			{
				printf("%d:%s\n",__LINE__,resultp[count]);
				if(j == 0) msg->info.id = atoi(resultp[count++]);
				if(j == 1) msg->info.type = atoi(resultp[count++]);
				if(j == 2) strcpy(msg->info.name,resultp[count++]);
				if(j == 3) strcpy(msg->info.password,resultp[count++]);
				if(j == 4) msg->info.age = atoi(resultp[count++]);
				if(j == 5) strcpy(msg->info.phone,resultp[count++]);
				if(j == 6) strcpy(msg->info.addr,resultp[count++]);
				if(j == 7) strcpy(msg->info.position,resultp[count++]);
				if(j == 8) strcpy(msg->info.date,resultp[count++]);
				if(j == 9) msg->info.level = atoi(resultp[count++]);
				if(j == 10) msg->info.salaray = atoll(resultp[count++]);
			}
		}
		printf("id = %d\n",msg->info.id);
		if(send(fd,msg,sizeof(MSG),0) < 0)
		{
			perror("query_name send error");
			return -1;
		}
	}

	return 0;
}

int user_modify(MSG *msg,int fd)
{
	char * errmsg;
	char sql[128];
	char sqlh[128];
	time_t t;
	struct tm *tp;
	
	time(&t);
	tp = localtime(&t);
	switch(msg->flags)
	{
		case 1:sprintf(sql, "update user_info set address='%s' where name='%s';",\
						msg->info.addr, msg->username);
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%s的地址为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,msg->username,msg->info.addr);
			   break;
		case 2:sprintf(sql, "update user_info set phone='%s' where name='%s';",\
					   msg->info.phone,msg->username);
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%s的电话为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,msg->username,msg->info.phone);
			   break;
		case 3:sprintf(sql, "update user_info set password='%s' where name='%s';",\
					   msg->info.password, msg->username);
			   sprintf(sqlh,"insert into history values('msg:%d-%d-%d %d:%d:%d------%s------%s修改%s的密码为%s');",\
					   tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec,msg->username,\
					   msg->username,msg->username,msg->info.password);
			   break;
	}
	if(sqlite3_exec(db,sql, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}
	if(sqlite3_exec(db,sqlh, NULL, NULL, &errmsg) != SQLITE_OK)
	{
		printf("%s\n",errmsg);
		return -1;
	}

	memset(msg->msg,0,sizeof(msg->msg));
	strcpy(msg->msg,"OK");
	if(send(fd,msg,sizeof(MSG),0) < 0)
	{
		perror("manager_modify send error");
		return -1;
	}
	return 0;
}

int main(int argc, const char *argv[])
{
	int ret;
	int confd;
	pid_t pid;
	char *errmsg;

	if(argc != 3)
	{
		fprintf(stderr,"Usage:%s <IP> <Port>\n",argv[0]);
		return -1;
	}

	if(sqlite3_open(DATABASE,&db) != SQLITE_OK)
	{
		printf("%s\n",sqlite3_errmsg(db));
		return -1;
	}
	else
	{
		printf("DATABASE open success\n");
	}
	if(sqlite3_exec(db,"create table user_info(id int primary key,type int,name text,password text,age int,phone text,address text,position text,date text,level int,salaray real);",NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s.\n",errmsg);
	}
	else
	{
		printf("create user_info table success\n");
	}

	if(sqlite3_exec(db,"create table history(info text);",NULL,NULL,&errmsg) != SQLITE_OK)
	{
		printf("%s.\n",errmsg);
	}
	else
	{
		printf("create history success\n");
	}

	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd == -1)
	{
		perror("fail to socket");
		return -1;
	}

	int on =1;
	if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on)) < 0)
	{
		perror("fail to setsockopt");
		return -1;
	}

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(atoi(argv[2])),
		.sin_addr.s_addr = inet_addr(argv[1]),
	};

#if 0	
	struct sockaddr_in cliaddr;
	memset(&cliaddr,0,sizeof(cliaddr));
	socklen_t addrlen = sizeof(cliaddr);
	confd = accept(sockfd,(struct sockaddr*)&cliaddr,&addrlen);
#endif

	ret = bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret  < 0)
	{
		perror("fail to bind");
		return -1;
	}
	
	ret = listen(sockfd,5);
	if(ret < 0)
	{
		perror("fail to listen");
		return -1;
	}

	signal(SIGCHLD,SIG_IGN);

	while(1)
	{
		confd = accept(sockfd,NULL,NULL);
		if(confd < 0)
		{
			perror("fail to accept");
			return -1;
		}
		pid = fork();
		if(pid < 0)
		{
			perror("fail to fork");
			return -1;
		}
		else if(pid == 0)
		{
			close(sockfd);
			do_client(confd);
		}
		else
		{
			close(confd);
		}

	}
	close(confd);
	close(sockfd);

	
	return 0;
}







