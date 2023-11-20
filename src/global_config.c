#include "global_config.h"

// Global instance of the configuration struct
static struct GlobalConfiguration global_config_instance;

struct GlobalConfiguration *get_global_configuration()
{
    return &global_config_instance;
}
