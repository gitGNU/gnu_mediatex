#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    struct in_addr addr;
    struct hostent *he;
    /* printf ("sizeof struct in_addr: %i\n", sizeof(struct in_addr)); */
    printf("in_addr: %s\n", argv[1]);
    addr.s_addr = inet_addr(argv[1]);
    if (addr.s_addr == 0 || addr.s_addr == -1)
    {
        perror("inet_addr");
        return 1;
    }
    /* else */
    he = gethostbyaddr(&addr, 4, AF_INET);
    if (he == 0)
    {
        perror("gethostbyaddr");
        switch (h_errno)
        {
            case HOST_NOT_FOUND:
                printf("HOST_NOT_FOUND\n");
                break;
            case NO_ADDRESS:
                printf("NO_ADDRESS\n");
                break;
                /* case NO_DATA: print ("NO_DATA\n"); break; */
            case NO_RECOVERY:
                printf("NO_RECOVERY\n");
                break;
            case TRY_AGAIN:
                printf("TRY_AGAIN\n");
                break;
        }
        return 1;
    }
    /* else */
    printf("%s\n", he->h_name);
    return 0;
}
