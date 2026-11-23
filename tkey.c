/*
 *  tkey.c
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    int c;
    FILE* file;
    if (argc==1)
        file = fopen("/dev/stdin","r"); 
    else
        file = fopen(argv[1], "r");
    if (file==0) {
        printf("nao abriu\n");
        exit(1);
    }
    while( !feof(file) ) {
        c = getc(file);
        printf("leu cod=%d, ch='%c'   \n", c, c);
        if ( c=='a' ) break;
    } 
    fclose(file);
}
