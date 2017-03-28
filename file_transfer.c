#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int tcp_send(int portno, char sendfile[100])
{
    int sockfd, new_fd, n, sin_size;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    struct stat filestat;
    FILE *fp;

    //TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
    {
        perror("socket");
        exit(1);
    }

    //Initail, bind to port
    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portno);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //binding
    if ( bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
    {
        perror("bind");
        exit(1);
    }

    //Start listening
    if ( listen(sockfd, 10) == -1 )
    {
        perror("listen");
        exit(1);
    }

    //Get file stat
    if ( lstat(sendfile, &filestat) < 0)
    {
        exit(1);
    }
    printf("The file size is %lu\n", filestat.st_size);
    fp = fopen(sendfile, "rb");

    //Connect
    memset(&their_addr, 0, sizeof(struct sockaddr_in));
    sin_size = 1;
    if ( (new_fd = accept(sockfd, (struct sockaddr*)&their_addr, &sin_size)) == -1 )
    {
        perror("accept");
        exit(1);
    }

    //Sending filename
    char buf1[100];
    n = write(new_fd, sendfile, 50);
    n = read(new_fd, buf1, 50);
    if (strcmp(buf1, "Recv filename")==0)
    {
        printf("Filename sent\n");
    }

    //Sending file
    int i,count=0;
    int size=filestat.st_size;
    size=size/20+1;
    char buf[size];
    while(!feof(fp))
    {
        time_t t1 = time(NULL);
        char *now = ctime(&t1);
        count=count+1;
        n = fread(buf, sizeof(char), sizeof(buf), fp);
        n = write(new_fd, buf, n);
        printf("Send %d percent  ",count*5);
        printf("%s",now);
        n = read(new_fd, buf1, 50);
    }
    if(count!=20)
    {
        for(i=count;i<20;i++)
        {
            count=count+1;
            time_t t1 = time(NULL);
            char *now = ctime(&t1);
            printf("Send %d percent  ",count*5);
            printf("%s",now);
        }
    }

    fclose(fp);
    close(new_fd);
    close(sockfd);
    return 0;
}

int tcp_recv(int portno, char a[100])
{
    int sockfd,n,i;
    struct sockaddr_in address;
    struct hostent *sender;
    FILE *fp;

    //TCP socket
    if ( ( sockfd = socket(AF_INET, SOCK_STREAM, 0) ) == -1 )
    {
        perror("socket");
        exit(1);
    }
    sender = gethostbyname(a);
    if (sender == NULL)
    {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    //Initial, connect to port
    bzero((char *) &address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(portno);
    bcopy((char *)sender->h_addr, (char *)&address.sin_addr.s_addr, sender->h_length);

    //Connect to server
    if ( connect(sockfd, (struct sockaddr*)&address, sizeof(struct sockaddr)) == -1)
    {
        perror("connect");
        exit(1);
    }

    //recv filename
    char buf1[100];
    n = read(sockfd, buf1, sizeof(buf1));
    n = write(sockfd, "Recv filename", sizeof(buf1));
    printf("Filename: [%s]\n",buf1);

    //Open file
    char file[100]="copy_";
    strncat(file, buf1, 100);
    printf("Copy filename: [%s]\n",file);
    if ( (fp = fopen(file, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    //Receive file from server
    int count=0;
    char buf[6000000];
    while(1)
    {
        time_t t1 = time(NULL);
        char *now = ctime(&t1);
        count=count+1;
        n = read(sockfd, buf, sizeof(buf));
        if(n == 0){break;}
        n = fwrite(buf, sizeof(char), n, fp);
        n = write(sockfd, "OK", 10);
        printf("Receive %d percent  ",count*5);
        printf("%s",now);
    }
    if(count<=20)
    {
        for(i=count;i<21;i++)
        {
            time_t t1 = time(NULL);
            char *now = ctime(&t1);
            printf("Receive %d percent  ",i*5);
            printf("%s",now);
        }
    }
    fclose(fp);
    close(sockfd);
    return 0;
}

int udp_recv()
{
    int sockfd,n;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    struct stat filestat;
    socklen_t theiraddr_len = sizeof(their_addr);
    FILE *fp;

    //UDP socket
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    //Initail, bind to port
    bzero((char *) &my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5188);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    //recv filename
    char buf1[100];
    bzero(buf1,100);
    n = recvfrom(sockfd, buf1, sizeof(buf1), 0,(struct sockaddr *) &their_addr, &theiraddr_len);
    n = sendto(sockfd, "Recv filename", 13, 0,(struct sockaddr *) &their_addr, theiraddr_len);
    printf("Filename: [%s]\n",buf1);

    //Open file
    char file[100]="copy_";
    strncat(file, buf1, 100);
    printf("Copy filename: [%s]\n",file);
    if ( (fp = fopen(file, "wb")) == NULL)
    {
        perror("fopen");
        exit(1);
    }

    //Receive size
    int i,c,count=0,loss=0,size=0,tmp=0;
    char num[100];
    n = recvfrom(sockfd, num, sizeof(num), 0,(struct sockaddr *) &their_addr, &theiraddr_len);
    size=atoi(num);
    n = sendto(sockfd, "Recv size", 9, 0,(struct sockaddr *) &their_addr, theiraddr_len);
    
    //Receive file
    char buff[100];
    while(1)
    {
        time_t t1 = time(NULL);
        char *now = ctime(&t1);        
        bzero(buff,sizeof(buff));
        c=c+1;
        n = sendto(sockfd, "OK", 2, 0,(struct sockaddr *) &their_addr, theiraddr_len);
        n = recvfrom(sockfd, buff, sizeof(buff), 0,NULL, NULL);
        if(strcmp(buff,"END")==0) {break;}
        n = fwrite(buff, sizeof(char), n, fp);
        tmp=tmp+100;
        if(tmp>size)
        {
            count=count+1;
            printf("Receive %d percent  ",count*5);     
            printf("%s",now);
            tmp=0;
        }
    }
    count=count+1;
    printf("Receive %d percent  ",count*5);
    time_t t1 = time(NULL);
    char *now = ctime(&t1);
    printf("%s",now);
    if(count<20)
    {
        printf("Receive failed\n");
    }
    int rate=(loss/c)*100;
    printf("Packet Loss Rate : %d percent\n",rate);

    //close file
    fclose(fp);
    close(sockfd);
    return 0;
}

int udp_send(char sendfile[100])
{
    int sockfd,n;
    struct stat filestat;
    struct sockaddr_in address;
    FILE *fp;
    socklen_t addresslen=sizeof(address);
    //UDP socket
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    //Initial, connect to port
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(5188);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");

    //Get file stat
    if ( lstat(sendfile, &filestat) < 0)
    {
        exit(1);
    }
    printf("The file size is %lu\n", filestat.st_size);
    fp = fopen(sendfile, "rb");

    //Sending filename
    char buf1[100];
    bzero(buf1,100);
    n = sendto(sockfd, sendfile, strlen(sendfile), 0, (struct sockaddr *) &address, addresslen);
    n = recvfrom(sockfd, buf1, sizeof(buf1), 0, (struct sockaddr *) &address, &addresslen);
    if (strcmp(buf1, "Recv filename")==0)
    {
        printf("Filename sent\n");
    }

    //Sending size
    int i,count=0,tmp=0;
    int size=filestat.st_size;    
    char num[100];
    size=size/20+1;
    sprintf(num, "%d",size);
    bzero(buf1,100);
    n = sendto(sockfd, num, strlen(num), 0, (struct sockaddr *) &address, addresslen);
    n = recvfrom(sockfd, buf1, sizeof(buf1), 0, (struct sockaddr *) &address, &addresslen);
    if (strcmp(buf1, "Recv size")!=0)
    {
        exit(1);
    }

    //Sending file
    char buf[100];
    while(!feof(fp))
    {
        time_t t1 = time(NULL);
        char *now = ctime(&t1);
        bzero(buf,sizeof(buf));
        bzero(buf1,sizeof(buf1));       
        n = recvfrom(sockfd, buf1, sizeof(buf1), 0, (struct sockaddr *) &address, &addresslen);
        n = fread(buf, sizeof(char), sizeof(buf), fp);
        n = sendto(sockfd, buf, n, 0, (struct sockaddr *) &address, addresslen);
        tmp=tmp+100;
        if(tmp>size)
        {
            count=count+1;
            printf("Send %d percent  ",count*5);     
            printf("%s",now);
            tmp=0;
        }

    }
    count=count+1;
    printf("Send %d percent  ",count*5);
    time_t t1 = time(NULL);
    char *now = ctime(&t1);
    printf("%s",now);
    sendto(sockfd,"END", 10, 0, (struct sockaddr *) &address, addresslen);
    /*if(count<20)
    {
        printf("Send failed\n");
    }*/

    //close file
    fclose(fp);
    close(sockfd);
    return 0;
}

int main(int argc, char *argv[])
{
    int portno;
    if(argc<5)
    {
        fprintf(stderr,"ERROR EXECUTION\n");
        exit(1);
    }
    if(strcmp(argv[1],"tcp")==0)//tcp
    {
        if(strcmp(argv[2],"send")==0)//send
        {
            printf("TCP Send\n");
            portno=atoi(argv[4]);
            tcp_send(portno,argv[5]);
        }
        if(strcmp(argv[2],"recv")==0)//recv
        {
            printf("TCP Receive\n");
            portno=atoi(argv[4]);
            tcp_recv(portno,argv[3]);
        }
    }
    if(strcmp(argv[1],"udp")==0)//udp
    {
        if(strcmp(argv[2],"send")==0)//send
        {
            printf("UDP Send\n");
            udp_send(argv[5]);
        }
        if(strcmp(argv[2],"recv")==0)//recv
        {
            printf("UDP Receive\n");
            udp_recv();
        }
    }
}
