/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** GetRecordedFiles.cpp
** 
** Cmdline utility to fast parse directory tree and return all video record files with date/time. 
**
** -------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <unistd.h>

#ifdef BUILD_GETRECORDEDFILES

static const char *kDateFileMask = "\?\?\?\?-\?\?-\?\?_\?\?-\?\?-\?\?.mkv";


void printHelp()
{
    printf("Get list of video archive records.\n"
       "Params:\n"
       "p [DIR_PATH] - set video archive dir path/print all archive unique dates\n"
       "f [DATE]     - print all records for specified date\n");
}


void printDates()
{
    glob_t info;
    
    char day1   = 0;
    char day2   = 0;
    char month1 = 0;
    char month2 = 0;
    char year1  = 0;
       
    if (glob(kDateFileMask, 0, NULL, &info) == 0) /* Must return sorted file array */
    {
        for (int i = 0; i < info.gl_pathc; ++i)
        {
            const char *file = info.gl_pathv[i];
            if (file[9] != day2   ||
                file[8] != day1   ||
                file[6] != month2 ||
                file[5] != month1 ||
                file[3] != year1)
            {
                fwrite(file, 10, sizeof(char), stdout);
                fwrite("\n", sizeof(char), 1, stdout);
                
                day2   = file[9];
                day1   = file[8];
                month2 = file[6];
                month1 = file[5];
                year1  = file[3];
            }
        }

        globfree(&info);
    }
}


void printFiles(const char *archDate)
{
    glob_t info;
    
    if (glob(kDateFileMask, 0, NULL, &info) == 0) /* Must return sorted file array */
    {
        for (int i = 0; i < info.gl_pathc; ++i)
        {
            const char *file = info.gl_pathv[i];
            if (file[9] == archDate[9] &&
                file[8] == archDate[8] &&
                file[6] == archDate[6] &&
                file[5] == archDate[5] &&
                file[3] == archDate[3])
            {
                fwrite(file + 11, 8, sizeof(char), stdout);
                fwrite("\n", sizeof(char), 1, stdout);
            }
        }

        globfree(&info);
    }
}


int main(int argc, char *argv[]) 
{
    if (argc < 3)
    {
        printHelp();
    }
    else
    {
        char *archDir = NULL;
        char *archDate = NULL;
        
        for (int i = 1; i < argc; ++i)
        {
            if (argv[i][0] == 'p' && i < argc - 1)
            {
                archDir = argv[++i];
            }
            else if (argv[i][0] == 'f' && i < argc - 1)
            {
                archDate = argv[++i];
            }
        }
        
        if (archDir == NULL)
        {
            printHelp();
        }
        else
        {
            chdir(archDir);
            
            if (archDate == NULL)
            {
                printDates();
            }
            else
            {
                printFiles(archDate);
            }
        }     
    }
    
    return 0;
}

#endif
