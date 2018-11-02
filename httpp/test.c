#include <stdio.h>

#include <avl/avl.h>
#include <igloo/httpp.h>


int main(int argc, char **argv)
{
    char buff[8192];
    int readed;
    igloo_http_parser_t parser;
    igloo_avl_node *node;
    igloo_http_var_t *var;

    igloo_httpp_initialize(&parser, NULL);

    readed = fread(buff, 1, 8192, stdin);
    if (igloo_httpp_parse(&parser, buff, readed)) {
        printf("Parse succeeded...\n\n");
        printf("Request was ");
        switch (parser.req_type) {
        case igloo_httpp_req_none:
            printf(" none\n");
            break;
        case igloo_httpp_req_unknown:
            printf(" unknown\n");
            break;
        case igloo_httpp_req_get:
            printf(" get\n");
            break;
        case igloo_httpp_req_post:
            printf(" post\n");
            break;
        case igloo_httpp_req_head:
            printf(" head\n");
            break;
        }
        printf("Version was 1.%d\n", parser.version);
        
        node = igloo_avl_get_first(parser.vars);
        while (node) {
            var = (igloo_http_var_t *)node->key;
            
            if (var)
                printf("Iterating variable(s): %s = %s\n", var->name, var->value);
            
            node = igloo_avl_get_next(node);
        }
    } else {
        printf("Parse failed...\n");
    }

    printf("Destroying parser...\n");
    httpp_destroy(&parser);

    return 0;
}


