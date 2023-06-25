/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** ReadWriteConfig.cpp
** 
** Cmdline utility to read or write values to ini-file. 
**
** -------------------------------------------------------------------------*/


#include<stdio.h>
#include <cstdlib>


#include "ConfigFile.h"


#ifdef BUILD_RWCONFIG

static void printHelp()
{
    printf("Read/Write ini-file utility\n"
        "Params:\n"
        "   <ini file path>\n"
        "   r/w   - read or write param\n"
        "   <section name>\n"
        "   <key name>\n"
        "   <value>\n\n"
        "rwconf \"myconf.ini\" r main mykey\n"
        "rwconf \"myconf.ini\" r main mykey main mykey2 main mykey3\n"
        "rwconf \"myconf.ini\" w main mykey mynewvalue\n"
        "rwconf \"myconf.ini\" w main mykey mynewvalue main mykey2 mynewvalue2\n"
    );
}

int main(int argc, char *argv[]) 
{
    bool needPrintHelp = false;

    if (argc > 4)
    {
        ConfigFile config;

        if (argv[2][0] == 'r')
        {
            config.reload(std::string(argv[1]));
            for (int i = 3; i <= argc - 2; i += 2)
            {
                printf((i == 3 ? "%s" : " %s"), config.getValue(std::string(argv[i]), std::string(argv[i + 1])).c_str());
            }

            printf("\n");
        }
        else if (argv[2][0] == 'w')
        {
            config.reload(std::string(argv[1]));
            for (int i = 3; i <= argc - 3; i += 3)
            {
                config.setValue(std::string(argv[i]), 
                    std::string(argv[i + 1]), std::string(argv[i + 2]));
            }

            config.save();
        }
        else
        {
            needPrintHelp = true;
        }
    }
    else
    {
        needPrintHelp = true;
    }

    if (needPrintHelp)
    {
        printHelp();
    }

    return 0;
}

#endif
