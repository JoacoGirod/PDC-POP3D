
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
user(char *s, struct Users *user)
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
        sem_init(&user->semaphore, 0, 1);
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
            "   -d <mail addr>   Prints version info.\n"
            "   -t '<transformation script>' Defines the transoformation script to be applied.\n"
            "\n",
            progname);
    exit(1);
}

void parse_args(const int argc, char **argv, Logger *logger)
{
    log_message(logger, INFO, ARGPARSER, "Parsing arguments");

    struct GlobalConfiguration *g_conf = get_global_configuration();

    g_conf->pop3_addr = "0.0.0.0";
    g_conf->pop3_port = 1110;

    g_conf->conf_addr = "127.0.0.1";
    g_conf->conf_port = 8080;

    int c;
    int nusers = 0;

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, 0, 'h'},
            {"pop3_addr", required_argument, 0, 'l'},
            {"conf_addr", required_argument, 0, 'L'},
            {"pop3_port", required_argument, 0, 'p'},
            {"conf_port", required_argument, 0, 'P'},
            {"user", required_argument, 0, 'u'},
            {"version", no_argument, 0, 'v'},
            {"maildir", required_argument, 0, 'd'},
            {"transformation", required_argument, 0, 't'}};

        c = getopt_long(argc, argv, "hl:L:Np:P:u:v:t:d:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            usage(argv[0]);
            break;
        case 'l':
            g_conf->pop3_addr = optarg;
            break;
        case 'L':
            g_conf->conf_addr = optarg;
            break;
        case 'p':
            g_conf->pop3_port = port(optarg);
            break;
        case 'P':
            g_conf->conf_port = port(optarg);
            break;
        case 'd':
            if (optarg != NULL)
            {
                strcpy(g_conf->maildir_folder, optarg);
            }
            else
            {
                fprintf(stderr, "Error: -d option requires an argument.\n");
                exit(1);
            }
            break;
        case 'u':
            if (nusers < MAX_USERS)
            {
                user(optarg, g_conf->users + nusers);
                g_conf->numUsers++;
                nusers++;
            }
            else
            {
                fprintf(stderr, "maximum number of command line users reached: %d.\n", MAX_USERS);
                exit(1);
            }
            break;
        case 't':
            if (optarg != NULL)
            {
                strcpy(g_conf->transformation_script, optarg);
            }
            else
            {
                fprintf(stderr, "Error: -t option requires an argument.\n");
                exit(1);
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
