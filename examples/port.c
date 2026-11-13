/* Parse a TCP port number from the command line.
 *
 *   cc port.c -o port && ./port 8080
 *   ./port 70000        -> invalid port "70000": number out of range
 *   ./port 80abc        -> invalid port "80abc": not a valid number
 */
#include <stdio.h>
#include "../strictnum.h"

int main(int argc, char **argv)
{
    sn_u16 port;
    sn_status st;

    if (argc != 2) {
        fprintf(stderr, "usage: %s PORT\n", argv[0]);
        return 2;
    }

    st = sn_parse_u16_z(argv[1], &port);
    if (st != SN_OK) {
        fprintf(stderr, "invalid port \"%s\": %s\n", argv[1], sn_status_string(st));
        return 1;
    }
    if (port == 0) {
        fprintf(stderr, "invalid port \"%s\": must be 1-65535\n", argv[1]);
        return 1;
    }

    printf("port = %u\n", (unsigned)port);
    return 0;
}
