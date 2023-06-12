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
    fprintf(stderr, "Usage %s -g -k KEY -v VALUE\n", command);
    fprintf(stderr, "Where k to set value, g to get the value\n");

    fprintf(stderr, "\t'n' night mode set to\n");
    fprintf(stderr, "\t\t'1' -> night mode on\n");
    fprintf(stderr, "\t\t'0' -> night mode off\n");
    fprintf(stderr, "\t\t'2' -> auto mode\n");
    fprintf(stderr, "\t\t'3' -> disable/manual config\n");
    fprintf(stderr, "\t'v' video day mode on/off\n");
    fprintf(stderr, "\t'f' flip image\n");
    fprintf(stderr, "\t'q' config file path\n");
    fprintf(stderr, "\t'r' day to night AWB\n");
    fprintf(stderr, "\t'a' day to night lum\n");
    fprintf(stderr, "\t'b' night ot day AWB\n");
    fprintf(stderr, "\t'd' night ot day lum\n");
    fprintf(stderr, "\t'e' ir cut on/off\n");
    fprintf(stderr, "\t'g' ir led on/off\n");
    fprintf(stderr, "\t'o' OSD text\n");
    fprintf(stderr, "\t'c' OSD front color\n");
    fprintf(stderr, "\t's' OSD font size\n");
    fprintf(stderr, "\t'x' OSD X pos\n");
    fprintf(stderr, "\t'y' OSD Y pos\n");
    fprintf(stderr, "\t'h' OSD alpha\n");
    fprintf(stderr, "\t'i' OSD back color\n");
    fprintf(stderr, "\t'l' OSD on/off\n");
    fprintf(stderr, "\t'm' motion detect sensitivity\n");
    fprintf(stderr, "\t'p' motion detect on/off\n");
}

#ifdef BUILD_SETCONF
int main(int argc, char *argv[]) {

    char key = 0;
    char *value = NULL;
    bool get = false;

    // Parse Arguments:
    int c;
    while ((c = getopt(argc, argv, "g:k:v:")) != -1) {
        switch (c) {
            case 'g':
                    get = true;
                    // no break on purpose, execute 'k' case
            case 'k':
                if (key == 0) 
                {
                    key = optarg[0];
                } 
                else 
                {
                       printf("Can not use g and k values at the same time\n", c);
                       usage(argv[0]);
                       exit(EXIT_FAILURE);
                }
                break;

            case 'v':
                value = optarg;
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

     switch (key) {
        case 'f':
            //TODO: flip
            break;
        case 'n':
            SETGETSHAREDMEMORYINT(conf->nightmode);
            break;
        case 'v':
            SETGETSHAREDMEMORYBOOL(conf->videoDay);
            break;
        case 'q':
            SETGETSHAREDMEMORYSTRING(conf->configFilePath);
            break;
        case 'r':
            SETGETSHAREDMEMORYINT(conf->dayNightAwb);
            break;
        case 'a':
            SETGETSHAREDMEMORYINT(conf->dayNightLum);
            break;
        case 'b':
            SETGETSHAREDMEMORYINT(conf->nightDayAwb);
            break;
        case 'd':
            SETGETSHAREDMEMORYINT(conf->nightDayLum);
            break;
        case 'e':
            SETGETSHAREDMEMORYBOOL(conf->irCut);
            break;
        case 'g':
            SETGETSHAREDMEMORYBOOL(conf->irLed);
            break;

        // OSD configuration
        case 'o':
            SETGETSHAREDMEMORYSTRING(conf->osdText);
            break;
        case 'c':
            SETGETSHAREDMEMORYINT(conf->osdFrontColor);
            break;
        case 's':
            SETGETSHAREDMEMORYINT(conf->osdFontSize);
            break;
        case 'x':
            SETGETSHAREDMEMORYINT(conf->osdX);
            break;
        case 'y':
            SETGETSHAREDMEMORYINT(conf->osdY);
            break;
        case 'h':
            SETGETSHAREDMEMORYINT(conf->osdAlpha);
            break;
        case 'i':
            SETGETSHAREDMEMORYINT(conf->osdBackColor);
            break;
        case 'j':
            SETGETSHAREDMEMORYINT(conf->osdEdgeColor);
            break;
        case 'l':
            SETGETSHAREDMEMORYINT(conf->osdEnabled);
            break;
        // Motion configuration
        case 'm':
            SETGETSHAREDMEMORYINT(conf->motionSensitivity);
            break;
        case 'p':
            SETGETSHAREDMEMORYINT(conf->motionEnabled);
            break;

    default:
        printf("Invalid Argument %c\n", key);
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    mem.writeConfig();

    return 0;
}

#endif