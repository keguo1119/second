#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>

//static char *dev = "/dev/second_mutex";
static char *dev = "/dev/second";

int main(int argc, char ** argv)
{
    int fd;
    int counter     = 0;
    int old_counter = 0;

    fd = open(dev, O_RDONLY);

    if(fd != -1)  {
        while(1) {
            usleep(800*1000);
            read(fd, &counter, sizeof(unsigned int));
            if(counter != old_counter)
            {
                printf("seconds after open /dev/second : %d\n", counter);
                old_counter = counter;
            }
        }
    } else {
        printf("Device open failed\n");
    }

    close(fd);

}