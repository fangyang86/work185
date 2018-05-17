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
void output_d(int v)
{
    if(v){
      *((unsigned long *) gvirt_addr_data) = gd1;
      mydelay_n1(0);
      *((unsigned long *) gvirt_addr_clk) = gc1;
      mydelay_n1(1);
      mydelay_n1(2);
      *((unsigned long *) gvirt_addr_clk) = gc0;
      return;
    }
    else{
      *((unsigned long *) gvirt_addr_data) = gd0;
      mydelay_n1(0);
      *((unsigned long *) gvirt_addr_clk) = gc1;
      mydelay_n1(1);
      mydelay_n1(2);
      *((unsigned long *) gvirt_addr_clk) = gc0;
      return;
    }
}
int main(int argc, char **argv) {
    unsigned long read_result, writeval;
    off_t target;
    off_t target_clk;
    off_t target_data,target_dir,target_status;// gpio7
    unsigned long v_data,v_dir,v_status,v_data0,v_data1;
    int access_type = 'w';
    int i;
    int ndelay=0;
    double d,d0,d1;
    char *pbuffer;
    int v;
	
    pbuffer = malloc(0x1400000);
    if(pbuffer==NULL){
      printf("memory not enough\n");
      return -1;
    }
    for(i=0;i<0x1400000;i++) pbuffer[i]=0x55;
    ndelay=atoi(argv[1]);

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
    v_dir |= 0xc7;//////////// gpio 0,1,2,6,7
    *((unsigned long *) gvirt_dir) = v_dir;
    v_data = *((unsigned long *) gvirt_data);
    v_data1 = v_data | 0x00c7;
    v_data0 = v_data & 0xffffff38;
    for(i=0;i<0x100000;i++){
        *((unsigned long *) gvirt_data) = v_data1;
        printf(" set 1\n")
        v_status = *((unsigned long *) gvirt_status);
        printf(" status: %08x\n",v_status)
        *((unsigned long *) gvirt_data) = v_data0;
        printf(" set 0\n")
        v_status = *((unsigned long *) gvirt_status);
        printf(" status: %08x\n",v_status)
    }
    printf(" loop set gpio7 end\n")
    




    read_result = *((unsigned long *) gvirt_addr_data);
    gd1=read_result | 0x200;
    gd0=read_result & 0xfffffdff;
    read_result = *((unsigned long *) gvirt_addr_clk);
    gc1=read_result | 0x200;
    gc0=read_result & 0xfffffdff;

    clock_gettime(CLOCK_REALTIME,&gts0);

    for(i=0;i<0x1000000;i++){
      *((unsigned long *) gvirt_addr_data) = gd1;
      *((unsigned long *) gvirt_addr_data) = gd0;
      //v = pbuffer[i] & 0x0ff;
#if 0
      output_d(v & 0x1);
      output_d(v & 0x2);
      output_d(v & 0x4);
      output_d(v & 0x8);
      output_d(v & 0x10);
      output_d(v & 0x20);
      output_d(v & 0x40);
      output_d(v & 0x80);
#endif
    }

    clock_gettime(CLOCK_REALTIME,&gts1);
    d0 = gts0.tv_sec + 0.000000001 * gts0.tv_nsec;
    d1 = gts1.tv_sec + 0.000000001 * gts1.tv_nsec;
    d = d1 - d0;
    printf(" time : %.9f \n",d);

    *((unsigned long *) gvirt_addr_data) = gd1;
    *((unsigned long *) gvirt_addr_clk) = gc1;
    if(munmap(gmap_base, MAP_SIZE) == -1) FATAL;
    close(gfd);
    free(pbuffer);
    return 0;
}

