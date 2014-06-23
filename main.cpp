/*
 * Copyright (C) 2014 Thorsten Wilmer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// g++ -std=c++11  main.cpp  -o make_sdimage    -D_FILE_OFFSET_BITS=64 
#include <stdio.h>
#include <string.h>
#include <cstdint>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>






struct partition_t
{
   unsigned char bootable;
   uint8_t head;
   uint8_t sector;
   uint8_t cylinder;
   unsigned char type;
   uint8_t head_end;
   uint8_t sector_end;
   uint8_t cylinder_end;
   uint32_t lba;
   uint32_t sectorcount;
   
   
} __attribute__((packed));
struct mbr_t {
  unsigned char booststrap[446];
  partition_t p[4];
  unsigned char signature1;
  unsigned char signature2;

} __attribute__((packed));


void long2chs(uint64_t ls, uint8_t &c, uint8_t &h, uint8_t &s)
{
   if(ls>=8422686720)
   {
      c=255;
      h=254;
      s=255;
      return;
   }
   ls=ls/512;

   s=ls % 63;
   s=s+1;
   ls=ls/63;
   h=ls%255;
   ls=ls/255;
   c=ls&0xFF;
   s|=(ls>>2) &0xC0;
   return ;

   ls=ls/512;
   int	spc = 255 * 63;

   int cyl	 = ls / spc;
   c=cyl;
   ls = ls % spc;
   h = ls / 63;
   s = ls % 63 ;	/* sectors count from 1 */
   cyl>>8;
   s++;
   cyl=cyl&0xFF;
   cyl=cyl << 6;
   s=s|cyl;
}

int main(int argc , char **argv)
{
  if(argc<3){
    printf("Produces an SDCARD image with up to four partitions of dos, linux or swap type\n");
    printf("make_sdiamge <image> [d|l] <partion1> [partition2]\n");
    return -1;
  } 
  mbr_t mbr;
  memset(&mbr,0, sizeof(mbr));
  const char *file=argv[1];
  printf("Wrie %s\n", file);
  int i=0;
  uint64_t start=512;
  mbr.signature1=0x55;
  mbr.signature2=0xAA;
  int out=open(file,O_CREAT| O_RDWR | O_CREAT ,  00666 );
  for(i=0;i<4 && 2+(i*2)+1<argc;i++)
  {
    char *inType=argv[2+(i*2)+0];
     char *inFile=argv[2+(i*2)+1];
     printf("Reading file %s\n", inFile);
           struct stat sb;
           if (stat(inFile, &sb) == -1) {
               perror("stat");
               exit(EXIT_FAILURE);
           }
           uint64_t size=sb.st_size;
           mbr.p[i].lba=start/512;
           long2chs(start, 
                 mbr.p[i].cylinder,
                 mbr.p[i].head ,
                 mbr.p[i].sector);

           printf(" Start %d  %d/%d/%d\n",(int) start, 
                 mbr.p[i].cylinder,
                 mbr.p[i].head ,
                 mbr.p[i].sector);
           mbr.p[i].type=0x0b;
           if(inType[0]=='l'){
              printf("Create linux partition\n");
              mbr.p[i].type=0x83;
           }else if(inType[0]=='s'){
              printf("Create swap partition\n");
              mbr.p[i].type=0x82;
           }else if(inType[0]=='d'){
              printf("Create dos partition\n");
              mbr.p[i].type=0x0c;
           }else{
              printf("Partition type not recognized %c\n", inType[0]);
           }
           mbr.p[i].sectorcount=(size/512)+1;
           long2chs(start+size ,
                 mbr.p[i].cylinder_end,
                 mbr.p[i].head_end ,
                 mbr.p[i].sector_end);
           printf(" Size %d End  %d/%d/%d\n",(int) mbr.p[i].sectorcount, 
                 mbr.p[i].cylinder_end,
                 mbr.p[i].head_end ,
                 mbr.p[i].sector_end);


           char buf[4096];
           /*lseek(out,512*(mbr.p[i].lba+mbr.p[i].sectorcount-1) ,SEEK_SET);
	   for(int i=0;i<512;i++)
           {
               buf[i]=0;
           }*/
           lseek(out,512*mbr.p[i].lba ,SEEK_SET);
           int in=open(inFile,O_RDONLY);
           int r=0;
           do{
             r=read(in,buf,sizeof(buf));
             if(r>0) 
               write(out,buf,r);
           } while(r>0);
           close(in);

           write(out,buf,512); 
         uint64_t new_start=((mbr.p[i].sector_end &0x3F )-1)*512 +
              mbr.p[i].head_end * 63 *512 +
              (mbr.p[i].cylinder_end | (mbr.p[i].sector_end &0xC0) <<2)*255*63*512;
        printf("New start %d Start %d\n",(uint32_t) new_start, (uint32_t)start);
        mbr.p[i].sectorcount=1+(new_start-start)/512;
        start=new_start+512;
           
  }
  lseek(out,start,SEEK_SET);
  char a=0;
  write(out,&a,1);
  lseek(out,0,SEEK_SET);
  write(out,&mbr,sizeof(mbr_t));
  close(out);

  uint64_t l=16450559l*512l;
  uint8_t c,h,s;
  long2chs(l,c,h,s);
  printf("chs %d %d %d\n",c,h,s);
  return 0;
}
