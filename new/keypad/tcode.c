#include <stdio.h>

main(int argc, char *argv[])
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
  while (!feof(file) ) {
    c1=getc(file);
    c2=getc(file);
    printf("leu cod=%04X   \n",c1*0x100+c2);
     if (c1==(char)1 && c1==c2) break;
  } 

  fclose(file);

}
