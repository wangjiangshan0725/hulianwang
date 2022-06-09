#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAXDATA 200
#define MAX_DATA 20000

typedef struct Email //邮件结构体
{
    char filename[MAXDATA]; //本地文件名
    char subject[MAXDATA];  //主题
    char content[MAX_DATA]; //解析后的明文正文
} Email;

char *student_code(char *pass);
int conn();
Email decode_retr(int sockfd, char *filename);
void save_as(Email email);
int search_text(char *keyword);
int display_subject();
unsigned char *base64_decode(unsigned char *code);

/*
用到的字符串处理：
·sprintf，将变量里的字符串格式化拼接到另一个字符串地址
·strcpy、strncpy，前者完全拷贝，后者只拷贝前n个字符到新指针变量
·strstr，在字符串里找子字符串，返回的是子串的指针地址，是内存里的位置，可以直接加减来取前后的字符
·strcmp、strncmp，前者全文匹配，后者匹配前n个，如果相等返回的是0
·memset，初始化字符串的每个位置内存为\0，防止出错
直接百度都有使用的例子
*/

/*
 创建pop3 socket连接并登录
 */
int conn()
{
    int sockfd;
    struct sockaddr_in s;
    char buf[MAXDATA];
    char **addrListPtr;
    char IP[32];
    struct hostent *host_ent;

    char user[MAXDATA];
    char pass[MAXDATA];
    char domain[MAXDATA];
    printf("Please input the POP3 domain:\n");
    printf(">>");
    scanf("%s", domain);
    getchar();

    //将域名转换为hostent、建立socket连接 https://blog.csdn.net/daiyudong2020/article/details/51946080
    if ((host_ent = gethostbyname(domain)) == NULL)
    {
        printf("dns failed.\n");
        exit(1);
    }
    addrListPtr = host_ent->h_addr_list;
    inet_ntop(host_ent->h_addrtype, *addrListPtr, IP, sizeof(IP)); //主机结构中的ip地址转换网络二进制结构到ASCII类型的地址

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket failed\n");
        exit(1);
    }

    s.sin_family = AF_INET;
    s.sin_port = htons(110); // POP3服务端口号110
    s.sin_addr.s_addr = inet_addr(IP);
    memset(&(s.sin_zero), 0, 8);

    //设置接收超时时间 https://blog.csdn.net/yufangbo/article/details/4398245
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 5000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout.tv_sec, sizeof(struct timeval));

    //创建连接
    if (connect(sockfd, (struct sockaddr *)&s, sizeof(struct sockaddr)) == -1)
    {
        printf("connect failed\n");
        exit(1);
    }
    else
    {
        // RFC1939 https://datatracker.ietf.org/doc/html/rfc1939#section-4
        //建立连接后会有一行欢迎信息，等待登录
        recv(sockfd, buf, MAXDATA, 0);
        printf("Welcome: %s\n", buf);
    }

    printf("Please input username:\n");
    printf(">>");
    scanf("%s", user);
    getchar();
    printf("Please input password:\n");
    printf(">>");
    student_code(pass);

    // RFC1939 https://datatracker.ietf.org/doc/html/rfc1939#page-13
    //也可以参看https://www.cnblogs.com/caodneg7/p/10430734.html
    //依次发送USER和PASS且检查POP3是否返回+OK
    char msg[MAXDATA];
    int len;
    memset(buf, 0, MAXDATA);

    len = sprintf(msg, "USER %s\r\n", user);
    send(sockfd, msg, len, 0);

    len = recv(sockfd, buf, MAXDATA, 0);

    if (strncmp(buf, "+OK", 3) != 0)
    {
        printf("%s", buf);
        exit(1);
    }

    len = sprintf(msg, "PASS %s\r\n", pass);
    send(sockfd, msg, len, 0);

    len = recv(sockfd, buf, MAXDATA, 0);
    printf("\nLogin: %s\n", buf);
    if (strncmp(buf, "+OK", 3) != 0)
    {
        exit(1);
    }
    else
    {
        return sockfd;
    }
}

/*
收retr指令的返回值，保存到文件，便于以行为单位读取
*/
Email decode_retr(int sockfd, char *filename)
{

    char buf[MAX_DATA];
    int fd, len;
    int pos;
    char *t;
    char *end_pos, *start_pos;
    if ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1)
    {
        exit(1);
    }

    memset(buf, 0, MAX_DATA);

    //是否已经接收到结束符
    int finish = 0;

    // RFC1939 https://datatracker.ietf.org/doc/html/rfc1939#page-8
    // 规范结合wireshark分析得，回复可能会有多个包，以+OK开始，结束符可能会单独发
    // 实际接收中socket可能会将多个包ACK后自动合并，也可能不会合并
    if ((len = recv(sockfd, buf, MAX_DATA, 0)) > 0)
    {
        if (strncmp(buf, "+OK", 3) == 0)
        {
            if ((t = strstr(buf, "\r\n")) != NULL)
            {
                t += 2;
                pos = (int)(t - buf);                             //找邮件体到消息体的长度
                if ((end_pos = strstr(buf, "\r\n.\r\n")) != NULL) //如果有结束符就结束，如果全在第一个报文，.前肯定有换行符
                {
                    finish = 1;
                    end_pos += 2;
                    end_pos[0] = '\0'; //截断.后内容
                    len -= 3;
                }
                write(fd, t, len - pos); //写从邮件体开始到结束的
            }
        }
        else
        { //如不为OK出错退出，打印返回的错误
            printf("%s\n", buf);
            exit(1);
        }
    }
    while (!finish && ((len = recv(sockfd, buf, MAX_DATA, 0)) > 0))
    {
        if (strncmp(buf, ".\r\n", 3) == 0)
        {
            finish = 1;
            break;
        }
        else if ((end_pos = strstr(buf, "\r\n.\r\n")) != NULL)
        {
            finish = 1;
            len -= 3;
            end_pos[0] = '\0';
        }
        write(fd, buf, len);
    }

    close(fd);

    //以行为单位读取，解码收到的文件

    // 结合RFC2045 https://datatracker.ietf.org/doc/html/rfc2045 与163,qq邮箱发出的MIME文本
    // 可得解码思路：
    //·读到行Subject: ，与换行符\r\n之间的为邮件主题
    //·我们只需要关心 text/plain 部分的正文，所以找行Content-Type: text/plain;，值得注意的是 charset= 有可能跟在同一行也可能在第二行，所以用行首，而不是全行匹配
    //·找如果有声明boundary，把boundary取出记下（multipart格式）
    //·找到Content-Type: text/plain;后，真正的正文在一个空行之后，如果找到那个空行，则开始读取。将读取到的多行拼接在一起。正文结尾可能有空行或者boundary作为结束，有结束则结束读取
    //·如果Content-Type: text/plain;后找到行Content-Transfer-Encoding: base64，则说明读取到的内容需要base64解码，结束读取后将正文送入base64解码函数

    char *TEXT_PLAIN = "Content-Type: text/plain;";
    char *SUBJECT = "Subject: ";
    char *BOUNDARY = "boundary=\"";
    char *ENCODING_BASE64 = "Content-Transfer-Encoding: base64\r\n";

    Email new_email;
    strncpy(new_email.filename, filename, strlen(filename));
    char now_read[MAX_DATA];
    char content[MAX_DATA];
    int text_plain = 0;
    int need_decode = 0;

    char *content_end = NULL;
    char *temp;

    int reading = 0;

    char boundary[MAXDATA];

    FILE *fp = fopen(filename, "r");

    memset(boundary, 0, MAXDATA);
    while (fgets(now_read, sizeof(now_read), fp) != NULL)
    {
        if (strncmp(now_read, SUBJECT, strlen(SUBJECT)) == 0)
        {
            start_pos = now_read + strlen(SUBJECT);
            len = strstr(now_read, "\r\n") - start_pos; //找到主题字符串长度，用末尾指针减行首指针
            memset(new_email.subject, 0, MAXDATA);
            strncpy(new_email.subject, start_pos, len);
            continue;
        }
        if ((temp = strstr(now_read, BOUNDARY)) != NULL)
        {
            start_pos = temp + strlen(BOUNDARY);
            len = strstr(now_read, "\"\r\n") - start_pos;
            strncpy(boundary, start_pos, len);
            continue;
        }

        if (strncmp(now_read, TEXT_PLAIN, strlen(TEXT_PLAIN)) == 0)
        {
            text_plain = 1;
            continue;
        }

        if (!strcmp(now_read, ENCODING_BASE64))
        {
            need_decode = 1;
            continue;
        }

        if (!reading && text_plain && strcmp(now_read, "\r\n") == 0) //如果已经找到过text/plain且找到空行
        {
            reading = 1;
            memset(content, 0, MAXDATA);
            content_end = content; //将待写入的地方置为content起始
            continue;
        }
        if (reading)
        {
            if ((boundary[0] != '\0' && strstr(now_read, boundary) != NULL) || strncmp(now_read, "\r\n", 2) == 0)
            {
                //如果在读取状态 且碰到boundary或空行则结束
                break;
            }

            int len = strlen(now_read) - 2;
            strncpy(content_end, now_read, len);
            content_end += len; //下一行待写入的地方是这一行结束后
        }
    }

    if (need_decode)
    {
        strcpy(new_email.content, base64_decode(content));
    }
    else
    {
        strcpy(new_email.content, content);
    }

    return new_email;
}

unsigned char *base64_decode(unsigned char *code)
{
    //根据base64表，以字符找到对应的十进制数据
    int table[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                   0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0,
                   63, 52, 53, 54, 55, 56, 57, 58,
                   59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0,
                   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                   13, 14, 15, 16, 17, 18, 19, 20, 21,
                   22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26,
                   27, 28, 29, 30, 31, 32, 33, 34, 35,
                   36, 37, 38, 39, 40, 41, 42, 43, 44,
                   45, 46, 47, 48, 49, 50, 51};
    long len;
    long str_len;
    unsigned char *res;
    int i, j;
    //计算解码后的字符串长度
    len = strlen(code);
    //判断编码后的字符串后是否有=
    if (strstr(code, "=="))
        str_len = len / 4 * 3 - 2;
    else if (strstr(code, "="))
        str_len = len / 4 * 3 - 1;
    else
        str_len = len / 4 * 3;

    res = malloc(sizeof(unsigned char) * str_len + 1);
    res[str_len] = '\0';

    //以4个字符为一位进行解码
    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {

        res[j] = ((unsigned char)table[code[i]]) << 2 | (((unsigned char)table[code[i + 1]]) >> 4);           //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合
        res[j + 1] = (((unsigned char)table[code[i + 1]]) << 4) | (((unsigned char)table[code[i + 2]]) >> 2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合
        res[j + 2] = (((unsigned char)table[code[i + 2]]) << 6) | ((unsigned char)table[code[i + 3]]);        //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合
    }

    return res;
}

/*
    不用getch()实现密码的输入，回显为*，最大可输入16位数
    https://blog.csdn.net/qq_44192078/article/details/104412489
*/
#define BACKSPACE 127 //删除的asccll码值
//使密码以*号输出
char *student_code(char *pass)
{
    int i = 0;
    system("stty -icanon"); //设置一次性读完操作，即getchar()不用回车也能获取字符
    system("stty -echo");   //关闭回显，即输入任何字符都不显示
    while (i < 16)
    {
        pass[i] = getchar(); //获取键盘的值到数组中
        if (i == 0 && pass[i] == BACKSPACE)
        {
            i = 0; //若开始没有值，输入删除，则，不算值
            pass[i] = '\0';
            continue;
        }
        else if (pass[i] == BACKSPACE)
        {
            printf("\b \b"); //若删除，则光标前移，输空格覆盖，再光标前移
            pass[i] = '\0';
            i = i - 1; //返回到前一个值继续输入
            continue;  //结束当前循环
        }
        else if (pass[i] == '\n') //若按回车则，输入结束
        {
            pass[i] = '\0';
            break;
        }
        else
        {
            printf("*");
        }
        i++;
    }
    system("stty echo");   //开启回显
    system("stty icanon"); //关闭一次性读完操作，即getchar()必须回车也能获取字符
    return pass;           //返回最后结果
}

/*另存为已解码的邮件主题、正文便于搜索与展示
emails.dat直接文本编辑器打开，格式如下：

[文件名]
主题
正文

如：
[e1.eml]
subject1
content line 1
content line 2222
[e2.eml]
subject222
content line 1content line

*/
void save_as(Email email)
{
    char *filename = "./emails.dat";
    FILE *fp;
    //如果存在就a追加，不存在就w新建
    if ((fp = fopen(filename, "a")) == NULL)
    {
        fp = fopen(filename, "w");
    }
    fprintf(fp, "[%s]\r\n%s\r\n%s\r\n", email.filename, email.subject, email.content);
    fclose(fp);
}

/*
搜索思路：
·如果第一个字符为'['且以']'和换行符结尾则为文件名
·接着取下一行即为主题
·不关注内容到底有多少行，啥时候截止，只需要在匹配到内容的时候输出上一次取到的文件名和主题
*/
int search_text(char *keyword)
{
    int total = 0; //总邮件文件数
    int found = 0; //包含关键字的文件数

    int len = 0;

    char now_read[MAXDATA];
    char *end_pos;
    char *start_pos;

    FILE *fp = fopen("./emails.dat", "r");
    if (fp == NULL)
    {
        printf("\nNo local email\n");
        return 0;
    }

    char now_filename[MAXDATA];
    char now_subject[MAXDATA];

    while (fgets(now_read, sizeof(now_read), fp) != NULL)
    {
        if ((now_read[0] == '[') && (end_pos = strstr(now_read, "]\r\n")) != NULL) //如果第一个字符为'['且以']'和换行符结尾则为文件名
        {
            memset(now_filename, 0, MAXDATA);
            memset(now_subject, 0, MAXDATA);

            start_pos = now_read + 1;
            len = end_pos - start_pos;
            strncpy(now_filename, start_pos, len);

            fgets(now_read, sizeof(now_read), fp); //接着取下一行去掉结尾即为主题
            len = strstr(now_read, "\r\n") - now_read;
            strncpy(now_subject, now_read, len);

            total++;
        }
        else
        {
            if (strstr(now_read, keyword) != NULL)
            {
                printf("The mail of %s has the text.\n", now_filename);
                found++;
            }
        }
    }

    printf("You have %d emails\n", total);
    printf("There is (are) %d mail(s) including the text '%s'\n", found, keyword);
    fclose(fp);
}

/*
展示思路：
·如果第一个字符为'['且以']'和换行符结尾则为文件名
·接着取下一行即为主题
·直接printf出来即可
*/
int display_subject()
{
    char now_read[MAXDATA];
    char *end_pos;
    char *start_pos;

    int len = 0;

    FILE *fp = fopen("./emails.dat", "r");
    if (fp == NULL)
    {
        printf("\nNo local email\n");
        return 0;
    }

    char now_filename[MAXDATA];
    char now_subject[MAXDATA];

    while (fgets(now_read, sizeof(now_read), fp) != NULL)
    {
        if ((now_read[0] == '[') && (end_pos = strstr(now_read, "]\r\n")) != NULL) //如果第一个字符为'['且以']'和换行符结尾则为文件名
        {
            memset(now_filename, 0, MAXDATA);
            memset(now_subject, 0, MAXDATA);

            start_pos = now_read + 1;
            len = end_pos - start_pos;
            strncpy(now_filename, start_pos, len);

            fgets(now_read, sizeof(now_read), fp); //接着取下一行去掉结尾即为主题
            len = strstr(now_read, "\r\n") - now_read;
            strncpy(now_subject, now_read, len);

            printf("the %s mail's Subject is: [%s]\n", now_filename, now_subject);
        }
    }

    fclose(fp);
}

int main()
{

    int sockfd = conn();
    int option;
    int len;
    char buf[MAXDATA];
    int email_number;
    Email email;
    char msg[MAXDATA];

    while (1)
    {
        printf("******************************************\n");
        printf("1.Get a list of messages and sizes\n");
        printf("2.Get mail status\n");
        printf("3.Display mail in detail\n");
        printf("4.Search text in all mails\n");
        printf("5.display by subjects\n");
        printf("6.Download the mail and delete in the remote service\n");
        printf("7.quit\n");
        printf("******************************************\n");
        printf("Please choose number:\n");
        printf(">>");
        scanf("%d", &option);

        memset(buf, 0, MAXDATA);
        memset(msg, 0, MAXDATA);

        switch (option)
        {
        case 1:
            len = sprintf(msg, "LIST\r\n");
            send(sockfd, msg, len, 0); //发LIST指令

            while ((len = recv(sockfd, buf, MAXDATA, 0)) > 0) // LIST的返回可能有多个包
            {
                printf("%s\n", buf);
                if (strstr(buf, "\r\n.") != NULL)
                    break;
                memset(buf, 0, MAXDATA);
            }

            break;
        case 2:
            len = sprintf(msg, "STAT\r\n");
            send(sockfd, msg, len, 0); // 发STAT指令，接收直接显示

            len = recv(sockfd, buf, MAXDATA, 0);
            printf("%s\n", buf);

            break;
        case 3:
            printf("Input email number to view in detail\n");
            printf(">>");
            scanf("%d", &email_number);

            len = sprintf(msg, "RETR %d\r\n", email_number);
            send(sockfd, msg, len, 0);

            email = decode_retr(sockfd, "temp.eml");

            //直接调用cat显示内容
            system("cat temp.eml");
            //显示完后rm删除
            system("rm temp.eml");

            printf("\nSubject: [%s]\nContent: [%s]\n\n", email.subject, email.content);

            break;
        case 4:
            printf("Please input the text you want to search:\n");
            printf(">>");
            scanf("%s", msg);
            search_text(msg);

            break;
        case 5:
            display_subject();
            break;
        case 6:
            printf("Input email number:\n");
            printf(">>");
            scanf("%d", &email_number);

            printf("Please input the filename you want to save:\n");
            printf(">>");
            char filename[50];
            scanf("%s", msg);
            sprintf(filename, "%s.eml", msg);

            len = sprintf(msg, "RETR %d\r\n", email_number);
            send(sockfd, msg, len, 0);

            email = decode_retr(sockfd, filename);
            save_as(email);

            //发送DELE指令，标记删除远端邮件
            len = sprintf(msg, "DELE %d\r\n", email_number);
            // send(sockfd, msg, len, 0); //!调试的时候可以注释此行以不删邮件

            printf("%s save successfully and delete from the remote server.\n", filename);

            break;
        case 7:
            close(sockfd);
            exit(0);
        default:
            printf("Invalid option\n");
            break;
        }
        printf("******************************************\n");
        printf("Press 1 to return to main interface\n");
        printf("Press 2 to quit\n");
        printf(">>");
        scanf("%d", &option);
        if (option != 1)
        {
            close(sockfd);
            exit(0);
        }
    }

    return 0;
}