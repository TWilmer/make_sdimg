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
   unsigned char head;
   unsigned char sector;
   unsigned char cylinder;
   unsigned char type;
   unsigned char sector_end;
   unsigned char cylinder_end;
   unsigned char type_end;
   uint32_t lba;
   uint32_t sectorcount;
   
   
} __attribute__((packed));
struct mbr_t {
  unsigned char booststrap[446];
  partition_t p[4];
  unsigned char signature1;
  unsigned char signature2;

} __attribute__((packed));

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
  uint64_t start=1;
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
           mbr.p[i].sectorcount=size/512;
           mbr.p[i].lba=start;
           start=start+mbr.p[i].sectorcount;
           mbr.p[i].head=0xFF;
           mbr.p[i].sector=0xFF;
           mbr.p[i].cylinder=0xFF;
           mbr.p[i].type=0x0b;
           if(inType[0]=='l')
              mbr.p[i].type=0x80;
           else if(inType[0]=='s')
              mbr.p[i].type=0x81;
           else if(inType[0]=='s')
              mbr.p[i].type=0x0b;
           else{
              printf("Partition type not recognized\n");
           }
           mbr.p[i].sector_end=0xFF;
           mbr.p[i].cylinder_end=0xFF;
   	   mbr.p[i].type_end=0xFF;
           lseek(out,512*mbr.p[i].lba ,SEEK_SET);
           int in=open(inFile,O_RDONLY);
           char buf[4096];
           int r=0;
           do{
             r=read(in,buf,sizeof(buf));
             if(r>0) 
               write(out,buf,r);
           } while(r>0);
           close(in);
           
  }
  lseek(out,0,SEEK_SET);
  write(out,&mbr,sizeof(mbr_t));
  close(out);

  return 0;
}
