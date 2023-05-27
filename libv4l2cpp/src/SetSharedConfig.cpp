/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** SharedConfig
** 
** Cmdline utility to get/set parameters for anyka camera. 
**
** -------------------------------------------------------------------------*/

#include<stdio.h>
#include <getopt.h>
#include <cstdlib>
#include "SharedMemory.h"


template<typename T>
void setSharedMemValue(T *param, const char *value)
{
    *param = (T)atoi(value);
}

template<typename T>
void getSharedMemValue(T param)
{
    printf("%d\n", (int)param);
}


#define SETGETSHAREDMEMORYINT(INT) if (get) printf("%d\n",  INT); else INT = atoi(value)
#define SETGETSHAREDMEMORYLONG(LONG) if (get) printf("%ld\n",  LONG); else LONG = atol(value)
#define SETGETSHAREDMEMORYSTRING(STR) if (get) printf("%s\n",  STR); else  strcpy(STR,value)
#define SETGETSHAREDMEMORYBOOL(INT) if (get) printf("%s\n",  INT?"true":"false"); else INT= strToBool(value)


template<typename T>
int stringToInts(char *str, T val[], size_t size)
{
    size_t i = 0;
    char *pt = strtok (str,",");
    while ((pt != NULL) && i < size) 
    {
        T a = (T)atoi(pt);
        val[i] = a;
        i++;
        pt = strtok (NULL, ",");
    }
    return (i == size);
}

bool strToBool(char *str)
{
    if (strcasecmp(str, "true") == 0) return(true);
    if (strcasecmp(str, "on") == 0) return(true);
    if (strcasecmp(str, "false") == 0) return(false);
    if (strcasecmp(str, "off") == 0) return(false);
    if (atoi(str) == 1) return (true);
    return false;
}

void usage(char *command)
{
    fprintf(stderr, "Usage %s -g -k KEY -v VALUE -m MODE -d DISABLEENABLE\n", command);
    fprintf(stderr, "Where k to set value, g to get the value, m for night/day mode: n or d, d for disable enable tune: 0 and 1\n");

    fprintf(stderr, "\t'n' night mode set to\n");
    fprintf(stderr, "\t\t'1' -> night mode on\n");
    fprintf(stderr, "\t\t'0' -> night mode off\n");
}

#ifdef BUILD_SETCONF
int main(int argc, char *argv[]) {

    char *key = 0;
    char *value = NULL;
    bool get = false;
    bool dayMode = true;
    bool disabelEnableTune = false;
    bool enableTune = false;

    // Parse Arguments:
    int c;
    while ((c = getopt(argc, argv, "g:k:v:m:d:")) != -1) {
        switch (c) {
            case 'g':
                    get = true;
                    // no break on purpose, execute 'k' case
            case 'k':
                if (key == 0) {
                    key = optarg;
                } else {
                       printf("Can not use g and k values at the same time\n", c);
                       usage(argv[0]);
                       exit(EXIT_FAILURE);
                }
                break;

            case 'v':
                value = optarg;
                break;

            case 'm':
                dayMode = optarg[0] == 'd';
                break;

            case 'd':
                disabelEnableTune = true;
                enableTune = optarg[0] != '0';
                break;


            default:
                printf("Invalid Argument %c\n", c);
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    SharedMemory &mem = SharedMemory::instance();
    SharedConfig *conf = mem.getConfig();

    mem.readConfig();

     if (strcmp(key, "n") == 0)     SETGETSHAREDMEMORYINT(conf->nightmode );    
    //else if (strcmp(key, "d") == 0)
    //    if (get) printf("%d,%d\n", conf->frmRateConfig[0], conf->frmRateConfig[1]);
    //    else stringToInts<int>(value, conf->frmRateConfig, 2);
    else
    {
        printf("Invalid Argument %c\n", key);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    mem.writeConfig();

    return 0;
}

#endif