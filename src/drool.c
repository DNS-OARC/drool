#include "config.h"

#include "drool.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char* program_name = 0;

static void usage(void) {
    printf(
        "usage: %s [-hV]\n"
        "  -h        Print this help and exit\n"
        "  -V        Print version and exit\n",
        program_name
    );
}

static void version(void) {
    printf("%s version " PACKAGE_VERSION "\n", program_name);
}

int main(int argc, char* argv[]) {
    int opt;

    if ((program_name = strrchr(argv[0], '/'))) {
        program_name++;
    }
    else {
        program_name = argv[0];
    }

    while ((opt = getopt(argc, argv, "hV")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                exit(0);

            case 'V':
                version();
                exit(0);

            default:
                usage();
                exit(2);
        }
    }
    return 0;
}
