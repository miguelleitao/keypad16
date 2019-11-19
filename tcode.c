#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  int c1, c2;
  FILE* file;
  if (argc==1)
    file = fopen("/dev/stdin","r"); 
  else
    file = fopen(argv[1], "r");
  if (file==0)
  {
	printf("nao abriu device file\n");
	exit(1);
  }
  while ( !feof(file) ) {
    c1=getc(file);
    if ( c1=='\n' ) continue;
    c2=getc(file);
    if ( c2=='\n' ) continue;

    //if ( c1!=0 || c2!=0 ) 
	    printf("leu cod=%04X   \n",c1*0x100+c2);
    if (c1==(char)1 && c1==c2) break;
  } 

  fclose(file);

}
