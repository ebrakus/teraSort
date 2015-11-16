#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include "config.h"

int self_id = 0;

long double find_sec_elapsed(struct timeval end, struct timeval start) {
    long double diff = end.tv_sec - start.tv_sec + ((float)(end.tv_usec - start.tv_usec))/1000000;
    return diff;
}

struct in_addr find_eth0_ip_address() {
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    //printf("%s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    return ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
}

char* build_domain_name(int serial_no) {
    char str1[] = "megasort";
    char str2[] = ".cloudapp.net";
    int str_len1 = strlen(str1);
    char* domain_name = malloc(DOMAIN_NAME_SIZE);
    char str_serial_no[15];

    memset(domain_name, 0, DOMAIN_NAME_SIZE);
    strcpy(domain_name, str1);
    sprintf(str_serial_no, "%d", serial_no);
    strcpy(domain_name + str_len1, str_serial_no);
    strcpy(domain_name + str_len1 + strlen(str_serial_no), str2);

    return domain_name;
}

int find_sockaddr(struct sockaddr_in* address, char* domain_name, int port) {
    struct addrinfo hints, *res;
    int status;
    char str_port[10];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    sprintf(str_port, "%d", port); 
    status = getaddrinfo(domain_name, str_port, &hints, &res);
    if(status != 0) {
        printf("Error in getaddrinfo %d\n", status);
        return -1;
    }

    memset(address, 0, sizeof(struct sockaddr_in));     /* Zero out structure */
    struct addrinfo *p = res;
    while(p != NULL) {
        if(p->ai_family == AF_INET) {
            address = ((struct sockaddr_in*)p->ai_addr);
            break;
        }
        p = p->ai_next;
    }

    // convert the IP to a string and print it:
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address->sin_addr, ipstr, sizeof(ipstr));
    printf("IP address = %s -- %s\n", ipstr, domain_name);
    return 0;
}

int find_id_from_hostname(char* hostname) {
    int i;
    char str_serial_no[15];
    char temp[15];
    char str_megasort[] = "megasort";
    int len_megasort = strlen(str_megasort);
    memcpy(temp, hostname, len_megasort);
    if(strncmp(temp, str_megasort, len_megasort) != 0) {
        return 0;
    }

    for(i = len_megasort; i < strlen(hostname); i++) {
        if(str_serial_no[i-len_megasort] == '.') {
            break;
        }
        str_serial_no[i - len_megasort] = hostname[i];
    }
    
    return atoi(str_serial_no);
}


void* run_server_thread(void* addr) {
    int listenfd = 0, connfd = 0;
    int n;
    char recvBuff[BATCH_SIZE];
    unsigned long long int sum=0;
    struct sockaddr_in* serv_addr = (struct sockaddr_in*)addr;

    /* Create a TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#if 0
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(info->ip_addr);//htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5001 + info->serial_no); 
#endif
    serv_addr->sin_addr = find_eth0_ip_address();

    /* Binding the socket to the appropriate IP and port */
    bind(listenfd, (struct sockaddr*)serv_addr, sizeof(struct sockaddr_in)); 
    listen(listenfd, 10); 

    /* Waiting for the client connection */
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 

    /* Take the starting measurement */
    struct timeval start, end;
    gettimeofday(&start, NULL);
    while((n = read(connfd, recvBuff, 1*1024*1024)) > 0)  //Receive 4 MB of data from the client
    {
        sum+=strlen(recvBuff);
        /* End measurement */
    }
    gettimeofday(&end, NULL);
    long double diff = find_sec_elapsed(end, start);
    double bw = (sum*8)/diff ;

    printf("\n\nBandwidth[%d: %d]:%f Mbps\n", self_id, ntohs(serv_addr->sin_port) - BASE_SERVER_PORT, bw);
    
    close(connfd);  //Closing the connection
    sleep(1);
    return NULL;
}

void* run_client_thread(void* num) {
    int j = 0;
    int rc = 0;
    int fd;
    struct sockaddr_in echoServAddr; 
    char echoString[BATCH_SIZE];
    int* serial_no = (int*)num;

    rc = find_sockaddr(&echoServAddr, build_domain_name(*serial_no), BASE_CLIENT_PORT + self_id);
    if(rc != 0) {
        printf("Unable to retrieve sokaddr %d\n", rc);
        return NULL;
    }

    /*
     * Create a 1MB of data to be sent to the server 
     */
    for(j = 0; j< BATCH_SIZE-1; j++){
        echoString[j] = 'h';
    }
    echoString[BATCH_SIZE-1] = 0;

    /* Create a TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if(fd == -1){
        printf("Can't open socket\n");
    }

    /* Establish the connection to the echo server */
    if (connect(fd, (struct sockaddr *)&echoServAddr, sizeof(struct sockaddr_in)) < 0){
        printf("Connect failed");
        return NULL;
    }
    printf("Connected\n");

    /*
     * Keep sending 1MB of data to the server continuously
     */
    int i = 0;
    while(i < 1000){
        j = send(fd, echoString, BATCH_SIZE, 0);
        i++;
    }
    /* Gracefully close the connection */
    shutdown(fd, 1);
    return NULL;
}

int main(int argc, char* argv[])
{
    int rc = 0, i;
    void *res;
    pthread_t client_thread[MESH_SIZE], server_thread[MESH_SIZE];
    int serial_no[MESH_SIZE];
    char self_hostname[DOMAIN_NAME_SIZE];
    struct sockaddr_in* serv_addr[MESH_SIZE];

    gethostname(self_hostname, DOMAIN_NAME_SIZE - 1);
    self_id = find_id_from_hostname(self_hostname);
    if(self_id == 0) {
        printf("Unexpected hostname: %s\n", self_hostname);
        return 0;
    }

    for(i = 1; i < MESH_SIZE+1; i++) {
        if(i == self_id) {
            continue;
        }
        serial_no[i - 1] = i; 
        rc = pthread_create(&client_thread[i - 1], NULL, run_client_thread, (void*)&serial_no[i - 1]);
        if(rc != 0) {
            printf("Failed to create client thread\n");
            return 0;
        }

        serv_addr[i - 1] = malloc(sizeof(struct sockaddr_in));
        if(find_sockaddr(serv_addr[i - 1], build_domain_name(self_id), BASE_SERVER_PORT + i) != 0) {
            printf("Unable to retrieve sockaddr\n");
            return 0;
        }
        rc = pthread_create(&server_thread[i - 1], NULL, run_server_thread, (void*)&serv_addr[i - 1]);
        if(rc != 0) {
            printf("Failed to create server thread\n");
            return 0;
        }
    }

    for(i = 1; i < MESH_SIZE+1; i++) {
        if(i == self_id) {
            continue;
        }
        pthread_join(client_thread[i-1], &res);
        pthread_join(server_thread[i-1], &res);
    }
    return 0;
}
