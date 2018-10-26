#include "../include/igloo/log.h"

#define CATMODULE "test"

#define LOG_ERR(x, y, z...) igloo_log_write(x, 1, CATMODULE "/" __FUNCTION__, y, ##z)


int main(void)
{
    int lid;

    igloo_log_initialize();

    lid = igloo_log_open("test.log");

    LOG_ERR(lid, "The log id is %d, damnit...", lid);

    igloo_log_close(lid);

    igloo_log_shutdown();
}
