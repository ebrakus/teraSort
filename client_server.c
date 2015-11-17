#include "utils.h"

int self_id = 0;

void* run_server_thread(void* other_no) {
    int listenfd = 0, connfd = 0;
    int n;
    char recvBuff[BATCH_SIZE];
    unsigned long long int sum=0;
    int* port = (int*)other_no;
    struct sockaddr_in serv_addr;

    /* Create a TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    serv_addr.sin_family = AF_INET;
    //serv_addr.sin_addr.s_addr = inet_addr(info->ip_addr);//htonl(INADDR_ANY);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(*port); 

    /* Binding the socket to the appropriate IP and port */
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)); 
    listen(listenfd, 10); 

    /* Waiting for the client connection */
    printf("Before accept\n");
    connfd = accept(listenfd, (struct sockaddr*)NULL, NULL); 
    printf("Wait over\n");

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
    double bw = ((sum*8)/diff)/(1024*1024);

    printf("Time elapsed = %Le\n", diff);
    printf("\n\nBandwidth[%d: %d]:%f Mbps\n", self_id, *port - BASE_SERVER_PORT, bw);
    
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

    //char ipstr[INET_ADDRSTRLEN];
    //inet_ntop(AF_INET, &echoServAddr.sin_addr, ipstr, sizeof(ipstr));

    printf("%d %d\n", ntohs(echoServAddr.sin_port), echoServAddr.sin_family);

    /* Construct the server address structure */
    //memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Zero out structure */
    echoServAddr.sin_family      = AF_INET;             /* Internet address family */
    //echoServAddr.sin_addr.s_addr = inet_addr(ipstr);   /* Server IP address */
    echoServAddr.sin_port        = htons(BASE_CLIENT_PORT + self_id); /* Server port */

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
        return NULL;
    }

    printf("Before connect\n");
    /* Establish the connection to the echo server */
    while (connect(fd, (struct sockaddr *)&echoServAddr, sizeof(struct sockaddr_in)) < 0){
        printf("Connect failed");
        sleep(1);
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
    int other_no[MESH_SIZE];
    char self_hostname[DOMAIN_NAME_SIZE];

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

        /*serv_addr[i - 1] = malloc(sizeof(struct sockaddr_in));
        if(find_sockaddr(serv_addr[i - 1], build_domain_name(self_id), BASE_SERVER_PORT + i) != 0) {
            printf("Unable to retrieve sockaddr\n");
            return 0;
        }*/
        other_no[i-1] = BASE_SERVER_PORT + i;
        rc = pthread_create(&server_thread[i - 1], NULL, run_server_thread, (void*)&other_no[i - 1]);
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
