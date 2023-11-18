#include <stdio.h>  /* for printf */
#include <stdlib.h> /* for exit */
#include <limits.h> /* LONG_MIN et al */
#include <string.h> /* memset */
#include <errno.h>
#include <getopt.h>

#include "args.h"

static unsigned short
port(const char *s)
{
    char *end = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s || '\0' != *end || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno) || sl < 0 || sl > USHRT_MAX)
    {
        fprintf(stderr, "Port should in in the range of 1-65536: %s\n", s);
        exit(1);
        return 1;
    }
    return (unsigned short)sl;
}

static void
user(char *s, struct users *user)
{
    char *p = strchr(s, ':');
    if (p == NULL)
    {
        fprintf(stderr, "Password not found\n");
        exit(1);
    }
    else
    {
        *p = 0;
        p++;
        user->name = s;
        user->pass = p;
    }
}

static void
version(void)
{
    fprintf(stderr, "POP3 version 0.0\n"
                    "ITBA Protocolos de Comunicación 2023/2 -- Grupo 13\n");
}

static void
usage(const char *progname)
{
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "\n"
            "   -h               Prints help.\n"
            "   -l <POP3 addr>   Address where the POP3 Server will be served.\n"
            "   -L <conf addr>   Address where the Configuration Service will be served.\n"
            "   -p <POP3 port>   POP3 Server connection port.\n"
            "   -P <conf port>   Configuration Service port.\n"
            "   -u <name>:<pass> User and Password for the POP3 Server (max 10).\n"
            "   -v               Prints version info.\n"
            "\n",
            progname);
    exit(1);
}

void parse_args(const int argc, char **argv, struct pop3args *args, Logger *logger)
{
    log_message(logger, INFO, ARGPARSER, "Parsing arguments");

    memset(args, 0, sizeof(*args)); // sobre todo para setear en null los punteros de users

    args->pop3_addr = "0.0.0.0";
    args->pop3_port = 1110;

    args->conf_addr = "127.0.0.1";
    args->conf_port = 8080;

    args->disectors_enabled = true;

    int c;
    int nusers = 0;

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {0, 0, 0, 0}};

        c = getopt_long(argc, argv, "hl:L:Np:P:u:v", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage(argv[0]);
            break;
        case 'l':
            args->pop3_addr = optarg;
            break;
        case 'L':
            args->conf_addr = optarg;
            break;
        case 'N':
            args->disectors_enabled = false;
            break;
        case 'p':
            args->pop3_port = port(optarg);
            break;
        case 'P':
            args->conf_port = port(optarg);
            break;
        case 'u':
            if (nusers >= MAX_USERS)
            {
                fprintf(stderr, "maximun number of command line users reached: %d.\n", MAX_USERS);
                exit(1);
            }
            else
            {
                user(optarg, args->users + nusers);
                nusers++;
            }
            break;
        case 'v':
            version();
            exit(0);
            break;
        default:
            fprintf(stderr, "unknown argument %d.\n", c);
            exit(1);
        }
    }
    if (optind < argc)
    {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc)
        {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);
    }

    log_message(logger, INFO, ARGPARSER, "Parsing Finished");
}
