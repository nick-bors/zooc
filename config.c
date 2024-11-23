#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

Config
getDefaultConf()
{
    return (Config){
        .min_scale = 0.01,
        .max_scale = 2.0,
        .drag_friction = 6.0,
        .scale_friction = 4.0,
        .scroll_speed = 1.5,
        .windowed = false
    };
}

void 
writeDefaultConf(FILE *f)
{
    fprintf(f, "min_scale      = 0.01\n");
    fprintf(f, "max_scale      = 2.0\n");
    fprintf(f, "scroll_speed   = 1.5\n");
    fprintf(f, "drag_friction  = 6.0\n");
    fprintf(f, "scale_friction = 4.0\n");
    fprintf(f, "windowed       = true\n");
}

Config
loadConf(const char *fileName)
{
    FILE *f = fopen(fileName, "r");
    if (f == NULL) {
        f = fopen(fileName, "wr");
        if (f == NULL)
            die("Unable to open config file %s", fileName);
        writeDefaultConf(f);
    }

    char *line = NULL;
    size_t len = 0;

    Config conf = getDefaultConf();
    while (getline(&line, &len, f) != -1) {
        char *c;
        c = strtok(line, " \t=");
        while (c != NULL) {
            /* c is argument */
            char *arg = c;

            c = strtok(NULL, " \t=");
            if (c == NULL)
                die(NULL, "Expected value for argument %s", arg);

            if (!strcmp(arg, "min_scale")) {
                conf.min_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "max_scale")) {
                conf.max_scale = strtof(c, NULL);
            } else if (!strcmp(arg, "scroll_speed")) {
                conf.scroll_speed = strtof(c, NULL);
            } else if (!strcmp(arg, "drag_friction")) {
                conf.drag_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "scale_friction")) {
                conf.scale_friction = strtof(c, NULL);
            } else if (!strcmp(arg, "windowed")) {
                switch (c[0]) {
                case 'f':
                case '0':
                    conf.windowed = false;
                    break;
                case 't':
                case '1':
                    conf.windowed = true;
                    break;
                default:
                    break;
                }
            } else {
                die("Unexpected configuration key '%s'", arg);
            }

            /* get next arg */
            c = strtok(NULL, " \t=");
        }
    }

    fclose(f);
    return conf;
}
