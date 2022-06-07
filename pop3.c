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
#include <string.h>

#define BUFF_SIZE 255
#define BUFF_SIZE_BIG 10240

#define POP3_PORT 110

typedef struct EmailStruct
{
    char name[BUFF_SIZE];        //本地文件名
    char subject[BUFF_SIZE];     //主题
    char content[BUFF_SIZE_BIG]; //解析后的明文正文
} Email;

typedef enum POP3_OPTS
{
    POP3_USER = 1,
    POP3_PASS,
    POP3_STAT,
    POP3_LIST,
    POP3_RETR,
    POP3_DELE,
    POP3_QUIT
} POP3_OPT;

// function declarations
int hiddenInput(char *password);
int connectPop3(char *domain, char *username, char *password);
int sendOpt(int sockfd, POP3_OPT opt, char *param);
Email retr(int sockfd, char *filename, char *param);
void saveTxt(char *domain, char *username, Email email);
int isOk(int sockfd);
int searchContent(char *domain, char *username, char *searching);
int dispSubject(char *domain, char *username);
int showList(int sockfd, char *param);
Email decodeEml(char *filename);

unsigned char *base64_encode(unsigned char *str);
unsigned char *base64_decode(unsigned char *code);

int main()
{
    int sockfd;
    char *te;
    char username[BUFF_SIZE];
    char password[BUFF_SIZE];
    char domain[BUFF_SIZE];
    printf("Input POP3 server domain:\n>>");
    scanf("%s", domain);
    getchar();
    printf("Input username:\n>>");
    scanf("%s", username);
    getchar();
    printf("Input password:\n>>");
    hiddenInput(password);
    if (domain[0] = '.')
        sprintf(domain, "pop3.126.com");
    if (username[0] = '.')
        sprintf(username, "hill010725");
    if (password[0] = '.')
        sprintf(password, "JPLGFRGKFPVJIXGU");

    sockfd = connectPop3(domain, username, password);
    int option;
    int selects;
    int len;
    char recvBuffer[BUFF_SIZE];
    char param[BUFF_SIZE];
    Email email;
    while (1)
    {
        printf("****************************\n");
        printf("1.Get a list of messages and sizes\n");
        printf("2.Get mail status\n");
        printf("3.Display mail in detail\n");
        printf("4.Search text in all mails\n");
        printf("5.display by subjects\n");
        printf("6.Download the mail and delete in the remote service\n");
        printf("7.quit\n");
        printf("****************************\n");
        printf("Please choose number:\n>>");
        scanf("%d", &option);
        if (option == 7)
        {
            close(sockfd);
            exit(0);
        }
        switch (option)
        {
        case 1:
            showList(sockfd, "");
            break;
        case 2:
            sendOpt(sockfd, POP3_STAT, "");
            len = recv(sockfd, recvBuffer, BUFF_SIZE, 0);
            printf("%s\n", recvBuffer);
            break;
        case 3:
            printf("Input email number to view in detail\n>>");
            scanf("%s", param);
            email = retr(sockfd, "./tmp.eml", param);
            system("cat ./tmp.eml");
            printf("\n\nDecoded Email Subject: %s\n\nContent: %s\n\n", email.subject, email.content);
            system("rm ./tmp.eml");
            break;
        case 4:
            printf("Please input the text you want to search:\n>>");
            scanf("%s", param);
            searchContent(domain, username, param);
            break;
        case 5:
            dispSubject(domain, username);
            break;
        case 6:
            printf("Input email number:\n>>");
            scanf("%s", param);
            printf("Please input the filename you want to save:\n>>");
            char filename[50];
            scanf("%s", filename);
            email = retr(sockfd, filename, param);
            saveTxt(domain, username, email);
            sendOpt(sockfd, POP3_DELE, param);
            printf("Save successfully and delete from the remote server.");
            break;
        default:
            printf("Invalid option\n");
            break;
        }
        int selectAfter;
        printf("Press 1 to return to main interface\n");
        printf("Press others to quit\n");
        printf(">>");
        scanf("%d", &selectAfter);
        if (selectAfter != 1)
        {
            close(sockfd);
            exit(0);
        }
    }

    return 0;
}
int connectPop3(char *domain, char *username, char *password)
{
    int sockfd;
    struct sockaddr_in sockAddr;
    char buf[BUFF_SIZE];
    char **addrListPtr;
    char ipAddr[32];
    struct hostent *hostPtr;

    if ((hostPtr = gethostbyname(domain)) == NULL)
    {
        printf("ERROR: failed to parse domain into host.");
        return 0;
    }
    addrListPtr = hostPtr->h_addr_list;
    inet_ntop(hostPtr->h_addrtype, *addrListPtr, ipAddr, sizeof(ipAddr));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("ERROR: Socket create failed");
    }

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(POP3_PORT);
    sockAddr.sin_addr.s_addr = inet_addr(ipAddr);
    memset(&(sockAddr.sin_zero), 0, 8);

    if (connect(sockfd, (struct sockaddr *)&sockAddr, sizeof(struct sockaddr)) == -1)
    {
        printf("ERROR: Connection create failed");
        exit(1);
    }
    else
    {
        recv(sockfd, buf, BUFF_SIZE, 0);
        printf("Connect: %s", buf);
    }

    if (sendOpt(sockfd, POP3_USER, username) == 1 && isOk(sockfd))
    {
        if (sendOpt(sockfd, POP3_PASS, password) == 1 && isOk(sockfd))
            return sockfd;
        else
        {
            printf("ERROR: Login failed. Please check your password.");
            exit(1);
        }
    }
    else
    {
        printf("ERROR: Login failed. Please check your username.");
        exit(1);
    }
}

int isOk(int sockfd)
{
    // return 1;
    int recvLen;
    char recvBuffer[BUFF_SIZE];
    memset(recvBuffer, 0, BUFF_SIZE);
    if ((recvLen = recv(sockfd, recvBuffer, BUFF_SIZE, 0)) <= 0)
        return 0;
    else
    {
        recvBuffer[recvLen] = '\0';
        if (strncmp(recvBuffer, "+OK", 3) == 0)
            return 1;
        else
            return 0;
    }
}

int sendOpt(int sockfd, POP3_OPT opt, char *param)
{
    char msg[BUFF_SIZE];
    memset(msg, 0, BUFF_SIZE);
    int msgLen;

    switch (opt)
    {
    case POP3_USER:
        msgLen = sprintf(msg, "USER %s\r\n", param);
        break;
    case POP3_PASS:
        msgLen = sprintf(msg, "PASS %s\r\n", param);
        break;
    case POP3_STAT:
        msgLen = sprintf(msg, "STAT\r\n");
        break;
    case POP3_LIST:
        msgLen = param[0] == '\0' ? sprintf(msg, "LIST\r\n") : sprintf(msg, "LIST %s\r\n", param);
        break;
    case POP3_RETR:
        msgLen = sprintf(msg, "RETR %s\r\n", param);
        break;
    case POP3_DELE:
        msgLen = sprintf(msg, "DELE %s\r\n", param);
        break;
    case POP3_QUIT:
        msgLen = sprintf(msg, "QUIT\r\n");
        break;
    }

    // printf("len:%d,msg::%s\n", msgLen, msg);
    if ((send(sockfd, msg, msgLen, 0)) != msgLen)
    {
        printf("ERROR: Send msg to POP3 server failed. Please re-login.\n");
        exit(1);
    }

    return 1;
}

Email retr(int sockfd, char *filename, char *param)
{

    char recvBuffer[BUFF_SIZE_BIG];
    int fd, len;
    int pos, total = -1;
    char *t;

    sendOpt(sockfd, POP3_RETR, param);

    if ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1)
    {
        printf("err:create %s failed\n", filename);
        exit(1);
    }
    bzero(recvBuffer, BUFF_SIZE_BIG);
    if ((len = recv(sockfd, recvBuffer, BUFF_SIZE_BIG, 0)) > 0)
    {
        if (strncmp(recvBuffer, "+OK", 3) == 0)
        {
            if ((t = strstr(recvBuffer, " ")) != NULL)
            {
                t++;
                total = atoi(t);
            }
            if ((t = strstr(recvBuffer, "\r\n")) != NULL)
            {
                t += 2;
                pos = (int)(t - recvBuffer);

                if ((len = write(fd, t, len - pos)) == (len - pos))
                    bzero(recvBuffer, BUFF_SIZE_BIG);
                pos = len;
            }
        }
    }
    while ((len = recv(sockfd, recvBuffer, BUFF_SIZE_BIG, 0)) > 0)
    {
        pos += len;
        if (pos >= total)
            break;
        if ((len = write(fd, recvBuffer, len)) == len)
            bzero(recvBuffer, BUFF_SIZE_BIG);

        else
            break;
    }
    if ((len = write(fd, recvBuffer, len - 3)) == len)
    {

        close(fd);

        return decodeEml(filename);
    }
    else
    {
        printf("ERROR: Writing file failed.\n");
        exit(1);
    }
}
int showList(int sockfd, char *param)
{
    char recvBuffer[BUFF_SIZE];
    int len;
    char tmp[BUFF_SIZE];
    char *t;

    sendOpt(sockfd, POP3_LIST, param);
    bzero(recvBuffer, BUFF_SIZE);
    while ((len = recv(sockfd, recvBuffer, BUFF_SIZE, 0)) > 0)
    {
        printf("%s\n", recvBuffer);
        if (strstr(recvBuffer, "\r\n.") != NULL)
            break;
        else
            strncpy(tmp, recvBuffer, len);
        bzero(recvBuffer, BUFF_SIZE);
    }
    if (len > 3)
        recvBuffer[len - 5] = '\0';
}
unsigned char *base64_encode(unsigned char *str)
{
    long len;
    long str_len;
    unsigned char *res;
    int i, j;
    //定义base64编码表
    unsigned char *base64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    //计算经过base64编码后的字符串长度
    str_len = strlen(str);
    if (str_len % 3 == 0)
        len = str_len / 3 * 4;
    else
        len = (str_len / 3 + 1) * 4;

    res = malloc(sizeof(unsigned char) * len + 1);
    res[len] = '\0';

    //以3个8位字符为一组进行编码
    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {
        res[i] = base64_table[str[j] >> 2];                                     //取出第一个字符的前6位并找出对应的结果字符
        res[i + 1] = base64_table[(str[j] & 0x3) << 4 | (str[j + 1] >> 4)];     //将第一个字符的后位与第二个字符的前4位进行组合并找到对应的结果字符
        res[i + 2] = base64_table[(str[j + 1] & 0xf) << 2 | (str[j + 2] >> 6)]; //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符
        res[i + 3] = base64_table[str[j + 2] & 0x3f];                           //取出第三个字符的后6位并找出结果字符
    }

    switch (str_len % 3)
    {
    case 1:
        res[i - 2] = '=';
        res[i - 1] = '=';
        break;
    case 2:
        res[i - 1] = '=';
        break;
    }

    return res;
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

int hiddenInput(char *password)
{
    // https://blog.csdn.net/qq_44192078/article/details/104412489
    int i = 0;
    system("stty -icanon"); //设置一次性读完操作，即getchar()不用回车也能获取字符
    system("stty -echo");   //关闭回显，即输入任何字符都不显示
    while (i < 16)
    {
        password[i] = getchar(); //获取键盘的值到数组中
        if (i == 0 && password[i] == '\b')
        {
            i = 0; //若开始没有值，输入删除，则，不算值
            password[i] = '\0';
            continue;
        }
        else if (password[i] == '\b')
        {
            printf("\b \b"); //若删除，则光标前移，输空格覆盖，再光标前移
            password[i] = '\0';
            i = i - 1; //返回到前一个值继续输入
            continue;  //结束当前循环
        }
        else if (password[i] == '\n') //若按回车则，输入结束
        {
            password[i] = '\0';
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
    printf("\n");
    return i;
}

int substr(char dst[], char src[], int start, int len)
{
    char *p = src + start; //定义指针变量指向需要提取的字符的地址
    int n = strlen(p);     //求字符串长度
    int i = 0;
    if (n < len)
    {
        len = n;
    }
    while (len != 0)
    {
        dst[i] = src[i + start];
        len--;
        i++;
    } //复制字符串到dst中
    dst[i] = '\0';
    return 0;
}

Email decodeEml(char *filename)
{
    Email a;
    strcpy(a.name, filename);
    char c[BUFF_SIZE_BIG];
    char content[BUFF_SIZE_BIG];
    int find_tp = 0;
    int find_kg = 0;
    int find_next_kg = 0;
    int decode = 0;
    FILE *fptr = fopen(filename, "r");
    char flag[] = "Content-Type: text/plain;";

    while (fgets(c, sizeof(c), fptr) != NULL)
    {
        if (c[0] == 'S' && c[1] == 'u')
        {
            int pos = strchr(c, '\n') - c - 1;
            int i = 0;
            substr(a.subject, c, 9, pos);
            char *find = strchr(a.subject, '\n');
            if (find)
                *find = '\0';
        }

        if (!find_kg)
        {
            if (!find_tp)
            {
                if (!strncmp(c, flag, 24))
                {
                    find_tp = 1;
                }
            }
            else
            {
                int pos = strchr(c, '\n') - c - 1;
                if (pos == 0)
                {
                    find_kg = 1;
                }
            }
        }
        else
        {
            if (!find_next_kg)
            {
                int pos = strchr(c, '\n') - c - 1;
                if (pos != 0)
                {
                    strcpy(content, c);
                }
                else
                {
                    find_next_kg = 1;
                }
            }
        }
        if (!strcmp(c, "Content-Transfer-Encoding: base64\r\n"))
        {
            decode = 1;
        }
    }
    char *rnPtr;
    if (rnPtr = strstr(content, "\n"))
    {
        rnPtr[0] = '\0';
    }
    printf("ctn:%s\n", content);
    if (decode == 1)
    {
        strcpy(a.content, base64_decode(content));
    }
    else
    {
        strcpy(a.content, content);
    }
    fclose(fptr);
    return a;
}

int searchContent(char *domain, char *username, char *searching)
{
    int cnt_email = 0;
    int cnt_find_email = 0;
    char filename[100];
    sprintf(filename, "%s_%s.txt", domain, username);
    char c[1000];
    char email_name[100];
    FILE *fptr1 = fopen(filename, "r");
    if (fptr1 == NULL)
    {
        printf("ERROR: No email have been downloaded, please download first.\n");
        exit(1);
    }
    int flag = 0;
    while (fgets(c, sizeof(c), fptr1) != NULL)
    {
        if (strstr(c, searching) != NULL && flag == 0 && strncmp(c, "-=-=boundary", 8))
        {
            cnt_find_email++;
            flag = 1;
        }
        else if (flag == 1 && !strncmp(c, "-=-=boundary", 8))
        {
            int begin = strchr(c, '$') - c + 1;
            int end = strchr(c, '*') - c;
            int len = end - begin;
            substr(email_name, c, begin, len);
            printf("find in %s\n", email_name);
            flag = 0;
            cnt_email++;
        }
        else if (flag == 0 && !strncmp(c, "-=-=boundary", 8))
        {
            cnt_email++;
        }
    }

    printf("total number of email %d\n", cnt_email);
    printf("find number of email %d\n", cnt_find_email);
    fclose(fptr1);
}

int dispSubject(char *domain, char *username)
{
    char filename[100];
    sprintf(filename, "%s_%s.txt", domain, username);
    char c[1000];
    FILE *fptr1 = fopen(filename, "r");
    if (fptr1 == NULL)
    {
        printf("ERROR: No email have been downloaded, please download first.\n");
        exit(1);
    }
    while (fgets(c, sizeof(c), fptr1) != NULL)
    {
        if (!strncmp(c, "-=-=boundary", 8))
        {
            char email_name[BUFF_SIZE];
            char email_sub[BUFF_SIZE];
            int begin = strchr(c, '*') - c + 1;
            int end = strchr(c, '\0') - c;
            int len = end - begin;
            //    		文件最后一行加换行符
            substr(email_sub, c, begin, len);

            begin = strchr(c, '$') - c + 1;
            end = strchr(c, '*') - c;
            len = end - begin;
            //    		文件最后一行加换行符
            substr(email_name, c, begin, len);
            printf("the %s mail's Subject is: %s", email_name, email_sub);
        }
    }
    fclose(fptr1);
}

void saveTxt(char *domain, char *username, Email email)
{
    char filename[BUFF_SIZE];
    sprintf(filename, "%s_%s.txt", domain, username);
    FILE *fp;
    fp = fopen(filename, "r");
    if (fp == NULL)
    {
        fp = fopen(filename, "w");
    }
    else
    {
        fp = fopen(filename, "a");
    }
    fprintf(fp, "%s\n-=-=boundary$%s*%s\n", email.content, email.name, email.subject);
    fclose(fp);
}