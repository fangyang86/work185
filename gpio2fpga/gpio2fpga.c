/*
 * devmem2.c: Simple program to read/write from/to any location in memory.
 *
 *  Copyright (C) 2000, Jan-Derk Bakker (jdb@lartmaker.nl)
 *
 *
 * This software has been developed for the LART computing board
 * (http://www.lart.tudelft.nl/). The development has been sponsored by
 * the Mobile MultiMedia Communications (http://www.mmc.tudelft.nl/)
 * and Ubiquitous Communications (http://www.ubicom.tudelft.nl/)
 * projects.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
  
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
#define MAP_SIZE 0x100000
#define MAP_MASK (MAP_SIZE - 1)

struct timespec gts0,gts1;
int gfd;
void *gmap_base,*gvirt_addr_data;
void *gvirt_addr_clk;
void *gvirt_data,*gvirt_dir,*gvirt_status;
unsigned long gd1,gd0;
unsigned long gc1,gc0;
unsigned long v_data,v_dir,v_status,v_data0,v_data1;
unsigned long gv_data,gv_dir,gv_status,gv_data0,gv_data1;
unsigned long v_dataLH,v_dataHH,v_dataLL,v_dataHL;

int clk_test()
{
  v_data = *((unsigned long *) gvirt_data);
  v_data0 = v_data & (~0x00c7);
  printf(" clk test loop    data:0x%08x    data0:0x%08x\n",v_data,v_data0);
  for(;;){
    v_data1 = v_data0 | 0x06;
    *((unsigned long *) gvirt_data) = v_data1;
    //mydelay(10);
    v_data1 = v_data0 | 0x46;
    *((unsigned long *) gvirt_data) = v_data1;
    //mydelay(10);
  }
  return 0;
}

int data_test()
{
  v_data = *((unsigned long *) gvirt_data);
  v_data0 = v_data & (~0x00c7);
  printf(" data test loop    data:0x%08x    data0:0x%08x\n",v_data,v_data0);
  for(;;){
    v_data1 = v_data0 | 0x06;
    *((unsigned long *) gvirt_data) = v_data1;
    v_data1 = v_data0 | 0x46;
    *((unsigned long *) gvirt_data) = v_data1;
    v_data1 = v_data0 | 0x06;
    *((unsigned long *) gvirt_data) = v_data1;
    v_data1 = v_data0 | 0x02;
    *((unsigned long *) gvirt_data) = v_data1;
    v_data1 = v_data0 | 0x42;
    *((unsigned long *) gvirt_data) = v_data1;
    v_data1 = v_data0 | 0x02;
    *((unsigned long *) gvirt_data) = v_data1;
  }
  return 0;
}
int mydelay(int n)
{
    int i,s=0;
    for(i=0;i<n;i++) s+=i;
    return s;
}
int mydelay_n1(int n)
{
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    return n+1;
}
int mydelay_n1nop(int n)
{
    asm("nop");
    asm("nop");
    return n+1;
}
inline void output_d(int v)
{
    if(v){
      //v_data1 = v_data0 | 0x06;
      *((unsigned long *) gvirt_data) = v_dataLH;
      //v_data1 = v_data0 | 0x46;
      *((unsigned long *) gvirt_data) = v_dataHH;
      return;
    }
    else{
      //v_data1 = v_data0 | 0x02;
      *((unsigned long *) gvirt_data) = v_dataLL;
      //v_data1 = v_data0 | 0x42;
      *((unsigned long *) gvirt_data) = v_dataHL;
      return;
    }
}
int main(int argc, char **argv) {
    unsigned long read_result, writeval;
    off_t target;
    off_t target_clk;
    off_t target_data,target_dir,target_status;// gpio7
    //unsigned long v_data,v_dir,v_status,v_data0,v_data1;
    unsigned long v_status1;
    int access_type = 'w';
    int i,j;
    double d,d0,d1;
    char *pbuffer,*p;
    int *pn;
    int v,len;
    FILE *fp;

    printf(" usage: gpio2fpga /root/fpga/led.rbf\n");
    if(argc<2){
      printf(" rbf file name missing\n");
      return 1;
    }

    fp = fopen(argv[1],"rb");
    if(fp==NULL){
      printf("  open file %s error\n",argv[1]);
      return 2;
    }

	
    pbuffer = malloc(0x1400000);
    if(pbuffer==NULL){
      printf("memory not enough\n");
      return -1;
    }

    len=fread(pbuffer,1,0xd00000,fp);
    printf(" file len:%d\n",len);
    fclose(fp);

    target_data=0x20b4000;
    target_dir=0x20b4004;
    target_status = 0x20b4008;


    target = 0x209c000;
    target_clk = 0x20ac000;

    if((gfd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    
    /* Map one page */
    gmap_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, gfd, target & ~MAP_MASK);
    if(gmap_base == (void *) -1) FATAL;

    gvirt_addr_data = gmap_base + (target & MAP_MASK);
    gvirt_addr_clk = gmap_base + (target_clk & MAP_MASK);
    gvirt_data = gmap_base + (target_data & MAP_MASK);
    gvirt_dir = gmap_base + (target_dir & MAP_MASK);
    gvirt_status = gmap_base + (target_status & MAP_MASK);

    v_dir = *((unsigned long *) gvirt_dir);
    v_dir &= ~0x00c7;//////////// gpio 0,1,2,6,7
    v_dir |= 0x46;//////////// gpio 0,1,2,6,7
    // gpio7.7 in....config_done
    // gpio7.6 out...dclk
    // gpio7.2 out....data
    // gpio7.1 out.....nConfig
    // gpio7.0 in.....nStatus
    *((unsigned long *) gvirt_dir) = v_dir;


    clock_gettime(CLOCK_REALTIME,&gts0);
    //data_test();/// loop
    //clk_test();/// loop

    v_data = *((unsigned long *) gvirt_data);
    v_data0 = v_data & (~0x00c7);
    //printf("         data0: 0x%08x\n",v_data0);
    v_dataLH = v_data0 | 0x06;
    v_dataHH = v_data0 | 0x46;
    v_dataLL = v_data0 | 0x02;
    v_dataHL = v_data0 | 0x42;

    //for(i=0;;i++){
    v_data1 = v_data0 | 0x0002;
    *((unsigned long *) gvirt_data) = v_data1;
    v_status = *((unsigned long *) gvirt_status);
    for(;;){
      v_status = *((unsigned long *) gvirt_status);
      printf("loop0.wait.status.high    status: %08x   bit.status %d\r",v_status,v_status & 1);
      if(1==(1&v_status))break;
    }
    printf("\n   nConfig.high ==> nStatus.high\n"); 

    v_data1 = v_data0 | 0x00;
    *((unsigned long *) gvirt_data) = v_data1;
    //for(j=0;j<0x10;j++){
    for(;;){
      v_status = *((unsigned long *) gvirt_status);
      printf("loop1.wait.status.low    status: %08x   bit.status %d\r",v_status,v_status & 1);
      if(0==(1&v_status))break;
    }
    printf("\n      nConfig.low  ==> nStatus.low \n");

    v_data1 = v_data0 | 0x0002;
    *((unsigned long *) gvirt_data) = v_data1;
    for(;;){
      v_status = *((unsigned long *) gvirt_status);
      printf("wait.status.high    status: %08x   bit.status %d\r",v_status,v_status & 1);
      if(1==(0x1&v_status))break;
    }
    printf("\n  \n");
    //}

    p  = pbuffer;
    for(i=0;i<len;i++){
      v = *p++;
      output_d(v & 0x1);
      output_d(v & 0x2);
      output_d(v & 0x4);
      output_d(v & 0x8);
      output_d(v & 0x10);
      output_d(v & 0x20);
      output_d(v & 0x40);
      output_d(v & 0x80);
    }
    printf("\n wait.done \n");
    for(i=0;;i++){
      v_status = *((unsigned long *) gvirt_status);
      //printf("wait.status.high    status: %08x   bit.status %d\r",v_status,v_status & 1);
      if(0x81==(0x81&v_status))break;
      if(0==(i&0x0fffff))printf(" %d status: 0x%08x\n",i,v_status);
      output_d(0);
    }
    printf("\n   download done \n");

    clock_gettime(CLOCK_REALTIME,&gts1);
    d0 = gts0.tv_sec + 0.000000001 * gts0.tv_nsec;
    d1 = gts1.tv_sec + 0.000000001 * gts1.tv_nsec;
    d = d1 - d0;
    printf(" time : %.9f \n",d);

exit_0:
    if(munmap(gmap_base, MAP_SIZE) == -1) FATAL;
exit_1:
    close(gfd);
exit_2:
    free(pbuffer);
    return 0;
}

