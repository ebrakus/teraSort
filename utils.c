#include "utils.h"

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
            memcpy(address, p->ai_addr, sizeof(struct sockaddr_in));
            break;
        }
        p = p->ai_next;
    }
    printf("%d\n", ntohs(address->sin_port));

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


