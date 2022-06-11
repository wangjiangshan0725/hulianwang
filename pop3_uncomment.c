#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>

#define BUFF_SIZE 255
#define BUFF_SIZE_BIG 65535

#define POP3_PORT 110
typedef struct EmailStruct
{
    char name[BUFF_SIZE];
    char subject[BUFF_SIZE];
    char content[BUFF_SIZE_BIG];
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

int connectPop3(char *domain, char *username, char *password);
int sendOpt(int sockfd, POP3_OPT opt, char *param);
Email retr(int sockfd, char *filename, char *param);
void saveTxt(char *domain, char *username, Email email);
int recvOk(int sockfd);
int searchContent(char *domain, char *username, char *searching);
int dispSubject(char *domain, char *username);
int showList(int sockfd, char *param);
Email decodeEml(char *filename);
int hiddenInput(char *password);
unsigned char *base64_decode(unsigned char *code);

void clean_stdin(void)
{
    int c;
    do
    {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

int main()
{
    int sockfd;
    char username[BUFF_SIZE];
    char password[BUFF_SIZE];
    char domain[BUFF_SIZE];
    printf("Input POP3 server domain:\n>>");
    scanf("%s", domain);
    clean_stdin();
    printf("Input username:\n>>");
    scanf("%s", username);
    clean_stdin();
    printf("Input password:\n>>");
    hiddenInput(password);
    if (domain[0] == '.')
        sprintf(domain, "pop3.126.com");
    else if (domain[0] == '/')
        sprintf(domain, "pop3.163.com");
    if (username[0] == '.')
        sprintf(username, "hill010725");
    else if (username[0] == '/')
        sprintf(username, "YikaiWang2001");
    if (password[0] == '.')
        sprintf(password, "JPLGFRGKFPVJIXGU");
    else if (password[0] == '/')
        sprintf(password, "THWSWPLWBPXQXPDO");

    sockfd = connectPop3(domain, username, password);
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
        printf("5.Display by subjects\n");
        printf("6.Download the mail and delete in the remote service\n");
        printf("7.Quit\n");
        printf("****************************\n");
        printf("Please choose number:\n>>");

        int opt = getchar();
        clean_stdin();

        switch (opt)
        {
        case '1':
            showList(sockfd, "");
            break;
        case '2':
            sendOpt(sockfd, POP3_STAT, "");
            len = recv(sockfd, recvBuffer, BUFF_SIZE, 0);
            printf("%s\n", recvBuffer);
            break;
        case '3':
            printf("Input email number to view in detail\n>>");
            scanf("%s", param);
            clean_stdin();

            email = retr(sockfd, "./tmp.eml", param);
            system("cat ./tmp.eml");
            printf("\n\nDecoded Email Subject: %s\n\nContent: %s\n\n", email.subject, email.content);
            system("rm ./tmp.eml");
            break;
        case '4':
            printf("Please input the text you want to search:\n>>");
            scanf("%s", param);
            clean_stdin();
            searchContent(domain, username, param);
            break;
        case '5':
            dispSubject(domain, username);
            break;
        case '6':
            printf("Input email number:\n>>");
            scanf("%s", param);
            clean_stdin();
            printf("Please input the filename you want to save:\n>>");
            char filename[50];
            scanf("%s", filename);
            clean_stdin();
            email = retr(sockfd, filename, param);
            saveTxt(domain, username, email);
            sendOpt(sockfd, POP3_DELE, param);
            printf("Save successfully and delete from the remote server.\n");
            break;
        case '7':
            close(sockfd);
            exit(0);
        default:
            printf("Invalid option\n");
            break;
        }
        printf("Press 1 to return to main interface\n");
        printf("Press others to quit\n");
        printf(">>");

        if (getchar() != '1')
        {
            close(sockfd);
            exit(0);
        }
        clean_stdin();
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
        exit(1);
    }
    addrListPtr = hostPtr->h_addr_list;

    inet_ntop(hostPtr->h_addrtype, *addrListPtr, ipAddr, sizeof(ipAddr));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("ERROR: Socket create failed");
        exit(1);
    }
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);

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

    if (sendOpt(sockfd, POP3_USER, username) == 1 && recvOk(sockfd))
    {
        if (sendOpt(sockfd, POP3_PASS, password) == 1 && recvOk(sockfd))
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

int recvOk(int sockfd)
{
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
        msgLen = sprintf(msg, "LIST\r\n");
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
    int pos;
    char *t, *end;
    sendOpt(sockfd, POP3_RETR, param);
    if ((fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1)
    {
        exit(1);
    }
    memset(recvBuffer, 0, BUFF_SIZE_BIG);
    int finishFlag = 0;
    if ((len = recv(sockfd, recvBuffer, BUFF_SIZE_BIG, 0)) > 0)
    {
        if (strncmp(recvBuffer, "+OK", 3) == 0)
        {
            if ((t = strstr(recvBuffer, "\r\n")) != NULL)
            {
                t += 2;
                pos = (int)(t - recvBuffer);
                if ((end = strstr(recvBuffer, "\r\n.\r\n")) != NULL)
                {
                    finishFlag = 1;
                    end += 2;
                    end[0] = '\0';
                    len -= 3;
                }
                write(fd, t, len - pos);
            }
        }
        else
        {
            printf("ERROR: Msg recv failed. Please re-connect.\n");
            exit(1);
        }
    }
    while (!finishFlag && ((len = recv(sockfd, recvBuffer, BUFF_SIZE_BIG, 0)) > 0))
    {

        if (strncmp(recvBuffer, ".\r\n", 3) == 0)
        {
            finishFlag = 1;
            break;
        }
        else if ((end = strstr(recvBuffer, "\r\n.\r\n")) != NULL)
        {
            finishFlag = 1;
            len -= 3;
            end[0] = '\0';
        }
        write(fd, recvBuffer, len);
    }

    close(fd);
    return decodeEml(filename);
}
int showList(int sockfd, char *param)
{
    char recvBuffer[BUFF_SIZE];
    int len;
    char tmp[BUFF_SIZE];
    char *t;

    sendOpt(sockfd, POP3_LIST, param);
    memset(recvBuffer, 0, BUFF_SIZE);
    while ((len = recv(sockfd, recvBuffer, BUFF_SIZE, 0)) > 0)
    {
        printf("%s\n", recvBuffer);
        if (strstr(recvBuffer, "\r\n.") != NULL)
            break;
        else
            strncpy(tmp, recvBuffer, len);
        memset(recvBuffer, 0, BUFF_SIZE);
    }
}
unsigned char *base64_decode(unsigned char *code)
{

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

    len = strlen(code);

    if (strstr(code, "=="))
        str_len = len / 4 * 3 - 2;
    else if (strstr(code, "="))
        str_len = len / 4 * 3 - 1;
    else
        str_len = len / 4 * 3;

    res = malloc(sizeof(unsigned char) * str_len + 1);
    res[str_len] = '\0';

    for (i = 0, j = 0; i < len - 2; j += 3, i += 4)
    {

        res[j] = ((unsigned char)table[code[i]]) << 2 | (((unsigned char)table[code[i + 1]]) >> 4);
        res[j + 1] = (((unsigned char)table[code[i + 1]]) << 4) | (((unsigned char)table[code[i + 2]]) >> 2);
        res[j + 2] = (((unsigned char)table[code[i + 2]]) << 6) | ((unsigned char)table[code[i + 3]]);
    }

    return res;
}

int hiddenInput(char *password)
{

    int i = 0;
    system("stty -icanon");
    system("stty -echo");
    while (i < 16)
    {
        password[i] = getchar();
        if (i == 0 && password[i] == '\b')
        {
            i = 0;
            password[i] = '\0';
            continue;
        }
        else if (password[i] == '\b')
        {
            printf("\b \b");
            password[i] = '\0';
            i = i - 1;
            continue;
        }
        else if (password[i] == '\n')
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
    system("stty echo");
    system("stty icanon");
    printf("\n");
    return i;
}

int substr(char dst[], char src[], int start, int len)
{
    char *p = src + start;
    int n = strlen(p);
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
    }
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
    int decode = 0;

    char *contentNowPos = NULL;
    char *temp;

    int reading = 0;
    char boundary[BUFF_SIZE];
    FILE *fptr = fopen(filename, "r");
    char *textFlag = "Content-Type: text/plain;";
    int textFlagLen = strlen(textFlag);
    char *subjectFlag = "Subject: ";
    int subjectFlagLen = strlen(subjectFlag);
    char *boundaryFlag = "boundary=\"";
    int boundaryFlagLen = strlen(boundaryFlag);
    char *base64Flag = "Content-Transfer-Encoding: base64\r\n";

    memset(boundary, 0, BUFF_SIZE);
    int ln = 0;
    while (fgets(c, sizeof(c), fptr) != NULL)
    {
        ln++;
        if (strncmp(c, subjectFlag, subjectFlagLen) == 0)
        {

            int len = strstr(c, "\r\n") - c - subjectFlagLen;
            memset(a.subject, 0, BUFF_SIZE);
            substr(a.subject, c, subjectFlagLen, len);
            continue;
        }
        if ((temp = strstr(c, boundaryFlag)) != NULL)
        {
            int len = strstr(c, "\"\r\n") - temp - boundaryFlagLen;
            substr(boundary, temp, boundaryFlagLen, len);
            continue;
        }

        if (strncmp(c, textFlag, textFlagLen) == 0)
        {
            find_tp = 1;
            continue;
        }

        if (!strcmp(c, base64Flag))
        {
            decode = 1;
            continue;
        }

        if (find_tp && strcmp(c, "\r\n") == 0)
        {
            memset(content, 0, BUFF_SIZE);

            contentNowPos = content;

            while ((fgets(c, sizeof(c), fptr) != NULL) && (boundary[0] == '\0' || strstr(c, boundary) == NULL) && strncmp(c, "\r\n", 2) != 0)
            {
                int len = strlen(c) - 2;
                strncpy(contentNowPos, c, len);

                contentNowPos += len;
            }
            break;
        }
    }
    strcpy(a.content, decode ? base64_decode(content) : (unsigned char *)content);
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

            substr(email_sub, c, begin, len);

            begin = strchr(c, '$') - c + 1;
            end = strchr(c, '*') - c;
            len = end - begin;

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