#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "base641.h"
#include <string.h>

#define BACKSPACE  0x08   //删除的asccll码值
#pragma comment(lib, "ws2_32.lib") 


#define PASSWORD_LEN 8 

#define MAXDATA 200
#define POP3_PORT 110

#define POP3_USER 1
#define POP3_PASS 2
#define POP3_STAT 3
#define POP3_LIST 4
#define POP3_RETR 5
#define POP3_DELE 6
#define POP3_QUIT 7

#define POP3_USER_PASS_ISEMPTY 10
#define POP3_USER_TO_LONG 11
#define POP3_pass_TO_LONG 12
#define POP3_USER_ERR 13
#define POP3_PASS_ERR 14 
#define POP3_NO_ONLINE  20

#define DEST_PORT 110//目标地址端口号 
// #define DEST_IP "123.126.97.79"/*目标地址IP，这里设为本机*/ 
#define MAX_DATA 65535//接收到的数据最大程度 
#define FILE_NAME   "temp.eml"
#define FILE_NAME1   "temp.txt"
#define FILE_NAME2   "temps.txt"
#define FILE_NAME3   "sear.txt"
#define FILE_NAME4   "req.txt"
#define BUF_LEN   65535

/*
	不用getch()实现密码的输入，回显为*，最大可输入16位数
*/
#define BACKSPACE  0x08   //删除的asccll码值
//使密码以*号输出
char* InputCode(char *pass)
{
	int i=0;
	system("stty -icanon");                   //设置一次性读完操作，即getchar()不用回车也能获取字符
	system("stty -echo");                     //关闭回显，即输入任何字符都不显示
	while(i < 16)
	{
		pass[i]=getchar();                    //获取键盘的值到数组中
		if(i == 0 && pass[i] == BACKSPACE)
		{
			i=0;                              //若开始没有值，输入删除，则，不算值
			pass[i]='\0';
			continue;
		}
		else if(pass[i] == BACKSPACE)
		{
			printf("\b \b");                    //若删除，则光标前移，输空格覆盖，再光标前移
			pass[i]='\0';
			i=i-1;                              //返回到前一个值继续输入
			continue;                           //结束当前循环
		}
		else if(pass[i] == '\n')               //若按回车则，输入结束
		{
			pass[i]='\0';
			break;
		}
		else
		{
			printf("*");
		}
		i++;
	}
	system("stty echo");                    //开启回显
	system("stty icanon");                   //关闭一次性读完操作，即getchar()必须回车也能获取字符
	return pass;                            //返回最后结果
}

int main()
{   
    int sockets;
    char *te;
    char testx[MAX_DATA];
    char user[MAX_DATA];
    char password[MAX_DATA];
    char domin[MAX_DATA];  
    printf("请输入域名：\n");
    scanf("%s",domin);
    getchar();
    printf("请输入用户名：\n");
    scanf("%s",user);
    getchar();
    printf("请输入密码：\n");
    // scanf("%s",password);
    // getchar()
	InputCode(password);
	printf("\n密码：%s\n",password);
	sockets=conn(&domin,&user,&password);
	 pop3_stat(sockets);
    int option;
    int selects;
    char names1[MAX_DATA];
    // int num1[];
    int num;
    while (1)
  {  
     menu();
     scanf("%d",&option); 
     // printf("输入的选项为：%d\n", option);
     if(option==7){
      printf("7.退出系统\n");
      close(sockets);
      break;
     }
     switch(option){
      case  1 :
      printf("1.查看邮箱邮件及邮件大小\n");
      pop3_list(sockets,0);
      selects=selectmenu();
      break;
      case  2 :
      printf("2.查看邮箱所有邮件及邮件大小\n");
      pop3_stat(sockets);
      selects=selectmenu();
      break; 
      case  3 :
      printf("3.查看邮件的具体内容\n");
      printf("请输入要查看的邮件序号：\n");
      scanf("%d",&num);
      pop3_retr(sockets,"./temp.eml",num);
      remo();
      base64();
      selects=selectmenu();
      break;
      case  4 :
      printf("4.搜索邮件内容\n");
      printf("请输入内容：\n");
      scanf("%s",testx);
      sear1(testx);
      sear2();
      selects=selectmenu();
      break;
      case  5 :
      printf("5.展示邮件主题\n");
      sub();
      selects=selectmenu();
      break;
      case  6 :
      printf("6.下载邮件\n");
      printf("请输入要下载的邮件序号：\n");
      scanf("%d",&num);
      sprintf(names1, "%d.eml",num);
      pop3_down(sockets,names1,num);
      pop3_retr(sockets,"./temp.eml",num);
      base641(te,num);
      printf("下载成功！~\n");
      selects=selectmenu();
      break;
      default:
      printf("无此选项，请重新选择\n");
      break;
      }
      if(selects==-1){
        printf("程序已退出！");
        close(sockets);
        break;
      }else if(selects==1){
        printf("返回主菜单！\n");
      }
     


    }
    
 
	return 0;
}



int conn(char *ptr1,char *user,char *pass)
{
    int sockfd,new_fd;/*cocket句柄和接受到连接后的句柄 */
    struct sockaddr_in dest_addr;/*目标地址信息*/
     char buf[MAX_DATA];//储存接收数据 
        char            *ptr, **pptr;
        char            IP[32];
        struct hostent  *hptr;
        ptr=ptr1;
        char *serverip=NULL; 
        
        if((hptr = gethostbyname(ptr)) == NULL)
        {
                printf("gethostbyname error for host:%s\n", ptr);
                return 0;
        }
        // printf("official hostname:%s\n",hptr->h_name);
        pptr = hptr->h_addr_list;
        serverip=(char*)inet_ntop(hptr->h_addrtype, *pptr, IP, sizeof(IP));

        // printf("test%s\n",serverip);
        // printf("IP address1111111:%s\n",inet_ntop(hptr->h_addrtype, *pptr, IP, sizeof(IP)));

        sockfd=socket(AF_INET,SOCK_STREAM,0);/*建立socket*/
        if(sockfd==-1){
        printf("socket failed:%d",errno);
    }
    
    //参数意义见上面服务器端 
    
    dest_addr.sin_family=AF_INET;
    dest_addr.sin_port=htons(DEST_PORT);
    dest_addr.sin_addr.s_addr=inet_addr(serverip);
    bzero(&(dest_addr.sin_zero),8);
    
    if(connect(sockfd,(struct sockaddr*)&dest_addr,sizeof(struct sockaddr))==-1){//连接方法，传入句柄，目标地址和大小 
        printf("connect failed:%d",errno);//失败时可以打印errno 
    } else{
        printf("connect success\n");
        recv(sockfd,buf,MAX_DATA,0);//将接收数据打入buf，参数分别是句柄，储存处，最大长度，其他信息（设为0即可）。 
        printf("Received:%s",buf);
    }
      if(pop3_login(sockfd,user,pass)!=1)
  {
    printf("me:login failed\n");  
    exit(1);
  }
  return sockfd;
}

int pop3_login_send(int sockfd,char *buf,int len,int flg)
{
  char cmd[50];
  bzero(cmd,50);  
  switch (flg)
  {
    case POP3_USER:
      len=snprintf(cmd,len+8,"USER %s\r\n",buf);      
    break;
    case POP3_PASS:
      len=snprintf(cmd,len+8,"PASS %s\r\n",buf);      
    break;    
  }
  if(send(sockfd,cmd,len,0)!=len)
  {
    printf("me:err cmd send\n");
    return 0;
  }
  else if(isok(sockfd))
    return 1;
  
}
int pop3_login(int sockfd,char *user,char *pass)
{
  int user_len=strlen(user);
  int pass_len=strlen(pass);  
  if(user_len<=0||pass_len<=0)
    return POP3_USER_PASS_ISEMPTY;
  if(user_len>40)
    return POP3_USER_TO_LONG;
  if(pass_len>40)
    return POP3_pass_TO_LONG;
  
  if(pop3_login_send(sockfd,user,user_len,POP3_USER)==1)
  {
    if(pop3_login_send(sockfd,pass,pass_len,POP3_PASS)==1)      
        return 1;
    else
    {
      pop3_login_send(sockfd,NULL,0,POP3_QUIT);
      return POP3_PASS_ERR;
    }
  }
  else 
    return POP3_USER_ERR;
}


int isok(int sockfd)
{
  int len=-1;
  char rec_buf[MAXDATA];
  bzero(rec_buf,MAXDATA); 
  if((len=recv(sockfd,rec_buf,MAXDATA,0))<=0)
    return 0;
  else
  {
    rec_buf[len]='\0';
    printf("me:%s\n",rec_buf);
    if(strncmp(rec_buf,"+OK",3)==0)
      return 1;
    else return 0;
  }
  
  
}



int pop3_updata(int sockfd,int number,int len,int flg)
{
  char cmd[50];
  char rec_buf[MAXDATA];
  // // printf("me:online%d\n",t_pop3->online);
  // if(t_pop3->online==0)
  //  return POP3_NO_ONLINE;
  bzero(cmd,50);
  bzero(rec_buf,MAXDATA);
  int len1;

  switch (flg)
  {
    case POP3_STAT:
      len1=snprintf(cmd,7,"STAT\r\n");  
    break;
    case POP3_LIST:
      if(number==0)
        len1=snprintf(cmd,7,"LIST\r\n");  
      else
        len1=snprintf(cmd,15,"LIST %d\r\n",number); 
    break;
    case POP3_RETR:
      if(number==0)
        len1=snprintf(cmd,9,"RETR 1\r\n");
      else
        len1=snprintf(cmd,15,"RETR %d\r\n",number);
    break;
    case POP3_DELE:
      if(number==0)
        len1=snprintf(cmd,7,"DELE\r\n");  
      else
        len1=snprintf(cmd,15,"DELE %d\r\n",number); 
    break;
    case POP3_QUIT:
      len1=snprintf(cmd,7,"QUIT\r\n");
    break;
  }
// printf("me:cmd%s\n",cmd);
  
  if((len1=send(sockfd,cmd,len1,0))<0)
  {
    printf("me:err cmd send\n");
    return 0;
  }
  else if(flg==POP3_DELE)
  {
      if(isok(sockfd)==1)
        return 1;
      else 
        return 0;
  } 
  else
    return 1;
}

int pop3_stat(int sockfd)
{
  char rec_buf[MAXDATA];
  int len;
  
  if(pop3_updata(sockfd,0,0,POP3_STAT)==1)
  {       
    bzero(rec_buf,MAXDATA);
    if((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
       printf("%s\n",rec_buf);  
      if(strncmp(rec_buf,"+OK",3)==0) 
//      {
//        //email_total=atoi(&rec_buf[4]);  
//      } 
      return 1;
  }
  else  
    return 0; 
}

int remo(){
  int   fd = -1;
  int   rv = -1;
  char  bufs[BUF_LEN];
  // unsigned char *bufn =NULL;



     //创建hello.txt文件
  if ((fd = open(FILE_NAME, O_RDWR|O_CREAT, 0666)) < 0)
  {
    printf("Open/Create file %s failure: %s\n", FILE_NAME, strerror(errno));
    return -1;
  }
  // printf("Open/Create file %s fd[%d] successfully\n", FILE_NAME, fd);
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs, sizeof(bufs))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  printf("Read %luB data from fd[%d]: %s\n", (long unsigned int)strlen(bufs), fd, bufs);
  // bufn = base64_decode(bufs);
  //       printf("%s\n",bufn);  
  //   }
  
cleanup:
  close(fd);

  /*移除临时文件*/  
   // memset(bufs, 0, sizeof(bufs));
   //   char *name;
   //   name = FILE_NAME ;
   //   remove(name);

  return 0;
}


int  sear1(char *tests){
   FILE * file2;
   file2=fopen("sear.txt","w");
   fputs(tests,file2);
   fclose(file2);
}


int  sear2(){
  int   fd = -1;
  int   rv = -1;
  char  bufs[BUF_LEN];
  char* cmd="sh test.sh sea";
  int status;
  status=system(cmd);
  // status=popen(cmd,"r");
  if(status==0){
    if ((fd = open(FILE_NAME4, O_RDWR|O_CREAT, 0666)) < 0)
  {
    printf("Open/Create file %s failure: %s\n", FILE_NAME4, strerror(errno));
    return -1;
  }
  // printf("Open/Create file %s fd[%d] successfully\n", FILE_NAME4, fd);
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs, sizeof(bufs))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  printf("******>>>>>>>>>搜索结果：%s\n",bufs);
  cleanup:
  close(fd);
  // 

  /*移除临时文件*/  
   memset(bufs, 0, sizeof(bufs));
     char *name;
     name = FILE_NAME4;
     char *name1;
     name1=FILE_NAME3;
     remove(name);

  }
  
   return 1;

}


int base64(){
  int   fd = -1;
  int   rv = -1;
  char  bufs[BUF_LEN];
  char* cmd="sh test.sh base";
  int status;
  status=system(cmd);
  if(status==0){
  unsigned char *bufn =NULL;
  // char filenames[MAX_DATA];
  // sprintf(filenames, "%d.txt",filenum);

   // if(access(filenames,0)==0){
    
   
  if ((fd = open(FILE_NAME1, O_RDWR|O_CREAT, 0666)) < 0)
  {
    printf("Open/Create file %s failure: %s\n", FILE_NAME, strerror(errno));
    return -1;
  }
  // printf("Open/Create file %s fd[%d] successfully\n", FILE_NAME, fd);
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs, sizeof(bufs))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  // printf("Read %luB data from fd[%d]: %s\n", strlen(bufs), fd, bufs);
  printf("**********************base64解码前正文为：**********************\n%s\n************************************************************\n",bufs);  
  printf("\n\n");
  bufn = base64_decode(bufs);
  printf("**********************base64解码后正文为：**********************\n%s\n************************************************************\n",bufn);  
  // bufn = base64_decode(bufs);
  //       printf("%s\n",bufn);  
  //   }
  
  cleanup:
  close(fd);
  
  /*移除临时文件*/  
   memset(bufs, 0, sizeof(bufs));
     char *name;
     name = FILE_NAME1;
     remove(name);
     char *name1;
     name1 = FILE_NAME;
     remove(name1);

 }
 // }else
 // printf("文件不存在\n");
 //     创建hello.txt文件

 //  return 0;
}

int  base641(char *te,int num){
  int   fd = -1;
  int   rv = -1;
  char  bufs[BUF_LEN];
  char  xs[BUF_LEN];
  FILE * file2;
  sprintf(xs, "%d.z",num);
  char* cmd="sh test.sh base";
  int status;
  status=system(cmd);
  if(status==0){
  unsigned char *bufn =NULL;
  // char filenames[MAX_DATA];
  // sprintf(filenames, "%d.txt",filenum);

   // if(access(filenames,0)==0){
    
   
  if ((fd = open(FILE_NAME1, O_RDWR|O_CREAT, 0666)) < 0)
  {
    printf("Open/Create file %s failure: %s\n", FILE_NAME, strerror(errno));
    return -1;
  }
  // printf("Open/Create file %s fd[%d] successfully\n", FILE_NAME1, fd);
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs, sizeof(bufs))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  // printf("Read %luB data from fd[%d]: %s\n", strlen(bufs), fd, bufs);
  printf("\n\n");
  bufn = base64_decode(bufs);
  // printf("**********************base64解码后正文为：\n%s\n**************************************\n",bufn);  
  // bufn = base64_decode(bufs);
  //       printf("%s\n",bufn);  
  //   }
  strcpy(te,(char *)bufn);
  file2=fopen(xs,"w");
  fputs(te,file2);
  cleanup:
  close(fd);
  fclose(file2);
  
  /*移除临时文件*/  
   memset(bufs, 0, sizeof(bufs));
     char *name;
     name = FILE_NAME1;
     remove(name);
     char *name1;
     name1 = FILE_NAME;
     remove(name1);

 }
 // }else
 // printf("文件不存在\n");
 //     创建hello.txt文件

  return 0;
}




int sub(){
  int   fd = -1;
  int   rv = -1;
  char  bufs1[BUF_LEN];
  char* cmd="sh test.sh sub";
  int status;
  status=system(cmd);
  if(status==0){
  unsigned char *bufn =NULL;
  // char filenames[MAX_DATA];
  // sprintf(filenames, "%d.txt",filenum);

   // if(access(filenames,0)==0){
  if ((fd = open(FILE_NAME2, O_RDWR|O_CREAT, 0666)) < 0)
  { 
    printf("Open/Create file %s failure: %s\n", FILE_NAME2, strerror(errno));
    return -1;
  }
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs1, sizeof(bufs1))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  printf("\n\n");
  // bufn = base64_decode(bufs);
  printf("\n下载邮件主题列表为：\n%s\n",bufs1);
  // bufn = base64_decode(bufs);
  //       printf("%s\n",bufn);  
  //   }
  
  cleanup:
  close(fd);
  
  /*移除临时文件*/  
   memset(bufs1, 0, sizeof(bufs1));
     char *name;
     name = FILE_NAME2;
     remove(name);
     // char *name1;
     // name1 = FILE_NAME;
     // remove(name1);

 }
 // }else
 // printf("文件不存在\n");
 //     创建hello.txt文件

 //  return 0;
}




int show(int filenum){
  int   fd = -1;
  int   rv = -1;
  char  bufs[BUF_LEN];
  char filenames[MAX_DATA];
  // unsigned char *bufn =NULL;
  sprintf(filenames, "%d.eml",filenum);

   if(access(filenames,0)==0){
    
   
  if ((fd = open(filenames, O_RDWR|O_CREAT, 0666)) < 0)
  {
    printf("Open/Create file %s failure: %s\n", FILE_NAME, strerror(errno));
    return -1;
  }
  // printf("Open/Create file %s fd[%d] successfully\n", FILE_NAME, fd);
  //从temp.eml中读数据
  lseek(fd, 0, SEEK_SET);
  if ((rv = read(fd, bufs, sizeof(bufs))) < 0)
  {
    printf("Read data from fd[%d] failure: %s\n", fd, strerror(errno));
    goto cleanup;
  }
  printf("Read %luB data from fd[%d]: %s\n", (long unsigned int)strlen(bufs), fd, bufs);
  // bufn = base64_decode(bufs);
  //       printf("%s\n",bufn);  
  //   }
  
cleanup:
  close(fd);

 }else
 printf("文件不存在\n");
     //创建hello.txt文件

  return 0;
}


int menu()
{
    
    printf("===================功能菜单===================\n");
    printf("1.查看邮箱邮件及邮件大小\n");
    printf("2.查看邮箱所有邮件及邮件大小\n");
    printf("3.查看邮件的具体内容\n");
    printf("4.搜索邮件内容\n");
    printf("5.展示邮件主题\n");
    printf("6.下载或删除邮件\n");
    printf("7.退出系统\n");
    printf("===================功能菜单===================\n");
    printf("请输入选项\n");

}

int selectmenu()
{ 
  int select;
  while(1){
  printf("1.返回上级菜单\n");
  printf("0.退出程序\n");
  printf("请输入选项：");
  scanf("%d",&select); 
  if(select==0){
    return -1;
    break;
  }else if(select==1){
    return 1;
    break;
  }
      switch(select){
      case 2:
        printf("无此选项，请重新选择\n");
        break;
      default:
        printf("无此选项，请重新选择\n");  
      }
     } 
     
 
}

int pop3_list(int sockfd,int number)
{
  char rec_buf[MAXDATA];
  int len,email_total=-1;
  char tmp[MAXDATA];
  char *t;

  
  if(pop3_updata(sockfd,number,0,POP3_LIST)==1)
  {       
    bzero(rec_buf,MAXDATA);
    while((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
    {
printf("me:%s\n",rec_buf);
      if(strstr(rec_buf,"\r\n.")!=NULL)
        break;
      else
        strncpy(tmp,rec_buf,len);
      bzero(rec_buf,MAXDATA); 
    }
    if(len>3)   
      rec_buf[len-5]='\0';
    else if(number==0)
      strncpy(rec_buf,tmp,len);
    if(strncmp(rec_buf,"+OK",3)==0) 
    {
      email_total=atoi(&rec_buf[4]);  
      //return email_total;
    }   
    // else if((t=strrchr(rec_buf,'\n'))!=NULL)
    // {     
    //   t++;
    //   email_total=atoi(t);
    // }
    // else 
    //   email_total = 0;
    // printf("me:totle %d\n",email_total);
    // return email_total;
  }
  else
    return 0;

}

int pop3_retr(int sockfd,char *path,int number)
{
  
  char rec_buf[MAXDATA];
  int fd,len;
  int pos,total=-1;
  char *t;
  

  if(pop3_updata(sockfd,number,0,POP3_RETR)==1)
  { 
    if((fd=open(path,O_CREAT|O_RDWR|O_TRUNC,0777))==-1)
    {
      printf("err:create %s failed\n",path);
      return 0;
    }
    bzero(rec_buf,MAXDATA);   
    if((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
    { 
      if(strncmp(rec_buf,"+OK",3)==0) 
      {
        if((t=strstr(rec_buf," "))!=NULL)
        {
          t++;
          total=atoi(t);  
        }
        if((t=strstr(rec_buf,"\r\n"))!=NULL)
        {
          t+=2;
          pos=(int)(t-rec_buf);
          
          if((len=write(fd,t,len-pos))==(len-pos))
            bzero(rec_buf,MAXDATA);
          pos=len;
        } 
      }
    }   
    while((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
    {   
      pos+=len;
      if(pos>=total)
        break;
      if((len=write(fd,rec_buf,len))==len)  
        bzero(rec_buf,MAXDATA);
  
      else
        break;  
    } 
    if((len=write(fd,rec_buf,len-3))==len)
    { 

      close(fd);

      return 1;
    }
  }
  else
    return 0;
  
  
}



//下载功能

int pop3_down(int sockfd,char *path,int number)
{
  
  char rec_buf[MAXDATA];
  int fd,len;
  int pos,total=-1;
  char *t;
  
  
  if(pop3_updata(sockfd,number,0,POP3_RETR)==1)
  { 
    if((fd=open(path,O_CREAT|O_RDWR|O_TRUNC,0777))==-1)
    {
      printf("err:create %s failed\n",path);
      return 0;
    }
    bzero(rec_buf,MAXDATA);   
    if((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
    { 
      if(strncmp(rec_buf,"+OK",3)==0) 
      {
        if((t=strstr(rec_buf," "))!=NULL)
        {
          t++;
          total=atoi(t);  
        }
        if((t=strstr(rec_buf,"\r\n"))!=NULL)
        {
          t+=2;
          pos=(int)(t-rec_buf);
          
          if((len=write(fd,t,len-pos))==(len-pos))
            bzero(rec_buf,MAXDATA);
          pos=len;
        } 
      }
    }   
    while((len=recv(sockfd,rec_buf,MAXDATA,0))>0)
    {   
      pos+=len;
      if(pos>=total)
        break;
      if((len=write(fd,rec_buf,len))==len)  
        bzero(rec_buf,MAXDATA);
  
      else
        break;  
    } 
    if((len=write(fd,rec_buf,len-3))==len)
    { 

      close(fd);

      return 1;
    }
  }
  else
    return 0;
  
  
}

