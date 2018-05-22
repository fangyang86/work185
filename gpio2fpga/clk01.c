#include<stdio.h>
#include<stdlib.h>

main()
{
   int i;
   char buf0[8];
   char buf1[8];

   buf0[0]='0';
   buf1[1]='1';
   FILE *fpclk;

   fpclk=fopen("/root/fpga/clk","wt");
   if(fpclk==NULL){
      printf(" open clk err\n");
      return;
   }
   for(;;){
     fprintf(fpclk,"0");
     fclose(fpclk);
   fpclk=fopen("/root/fpga/clk","wt");
     fprintf(fpclk,"1");
     fclose(fpclk);
   fpclk=fopen("/root/fpga/clk","wt");
     //fwrite(buf0,1,1,fpclk);
     //fwrite(buf0,1,1,fpclk);
   }

   fclose(fpclk);

   return;

}
