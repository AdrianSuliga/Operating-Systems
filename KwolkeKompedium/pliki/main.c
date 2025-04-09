#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>  
#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>     
#include <string.h>     

/* 
Wstaw we wskazanych miejscach odpowienie deskryptory, sposrod 
fd0, fd1, fd2, by uzyskac na terminalu nastepujacy efekt dzialania 
programu:

Hello,
Hello, 12345678
HELLO, 12345678
WITAM! 12345678
*/

int
main(int argc, char *argv[])
{
    int fd0, fd1, fd2; 
#define file "a"
    char cmd[] = "cat " file "; echo";

    fd1 = open(file, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR); // open for writing (truncate)
    if (fd1 == -1)
       {
        fprintf(stderr, "open fd1");
        exit(-1);
       }

    fd2 = dup(fd1); // duplicate fd1 to fd2
    if (fd2 == -1)
     {
        fprintf(stderr, "dup");
        exit(-1);
     }     

    fd0 = open(file, O_RDWR); // open for reading and writing
    if (fd0 == -1)
    {
        fprintf(stderr, "open fd0");
        exit(-1);
     } 
      
    if (write(fd1, "Hello,", 6) == -1)  // write "Hello," at the beginning
    {
        fprintf(stderr, "write1");
        exit(-1);
     }  
    system(cmd);

    if (write(fd2, " 12345678", 9) == -1) // append " 12345678"
    {
        fprintf(stderr, "write2");
        exit(-1);
     }  
    system(cmd);

    if (lseek(fd0, 0, SEEK_SET) == -1) // move fd0 pointer to the beginning
    {
        fprintf(stderr, "lseek");
        exit(-1);
     }  

    if (write(fd0, "HELLO,", 6) == -1) // overwrite beginning with "HELLO,"
    {
        fprintf(stderr, "write3");
        exit(-1);
     }  

    system(cmd);

    if (lseek(fd0, 0, SEEK_SET) == -1) // move fd0 pointer to the beginning
    {
        fprintf(stderr, "lseek");
        exit(-1);
     }  

    if (write(fd0, "WITAM!", 6) == -1) // overwrite beginning with "WITAM!"
    {
        fprintf(stderr, "write4");
        exit(-1);
     }  

    system(cmd);

    return 0;
}

