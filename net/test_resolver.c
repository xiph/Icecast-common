#include <stdio.h>
#include <stdlib.h>

#include "resolver.h"

int main()
{
    char buff[1024];

    igloo_resolver_initialize();

    printf("I got %s, when looking up %s.\n", igloo_resolver_getip("bach.greenwitch.com", buff, 1024), "bach.greenwitch.com");
    printf("I got %s, when looking up %s.\n", igloo_resolver_getname("207.181.249.14", buff, 1024), "207.181.249.14");

    return 0;
}

