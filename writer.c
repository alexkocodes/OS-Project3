/* Group Parners: Alex Ko (fyk211) and Ritin Malhotra (rm5486)*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>

int main(int argc, char *argv[])
{
    char *filename;
    char *recid;
    int time;
    char *shmid;

    int i = 0;
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            filename = argv[i + 1];
            printf("%s", filename);
        }
        if (strcmp(argv[i], "-l") == 0)
        {
            recid = argv[i + 1];
            printf("%s", recid);
        }
        if (strcmp(argv[i], "-d") == 0)
        {
            time = atoi(argv[i + 1]);
            printf("%d", time);
        }
        if (strcmp(argv[i], "-s") == 0)
        {
            shmid = argv[i + 1];
            printf("%s", shmid);
        }
    }
}
