#include "global_config.h"

// Global instance of the configuration struct
static struct GlobalConfiguration global_config_instance;

static struct GlobalStatistics global_stats_instance;

static struct ConfigServer config_server;

struct GlobalConfiguration *get_global_configuration()
{
    return &global_config_instance;
}

struct GlobalStatistics *get_global_statistics()
{
    return &global_stats_instance;
}

struct ConfigServer *get_config_server()
{
    return &config_server;
}
