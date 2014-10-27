#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

const char* file1 = "/dev/sdb2";
const char* file2 = "/dev/sdd2";
const char* file3 = "/dev/md0";

/* start comparing at this offset into the files */
const size_t offset1 = 262144 * 512; //calculated via  mdadm --examine file1 
const size_t offset2 = 262144 * 512; //calculated via  mdadm --examine file2
const size_t offset3 = 0;

const size_t block_size = 1024*128;

const int fs_block_size = 4096; //found via tune2fs -l

long count_bits(long n) {     
  unsigned int c; // c accumulates the total bits set in v
  for (c = 0; n; c++) 
    n &= n - 1; // clear the least significant bit set
  return c;
}

int main(int argc, char **argv)
{
  FILE *f1 = fopen(file1, "r");
  if (!f1)
  {
    printf("Failed to open %s: %s\n", file1, strerror(errno));
    exit(1);
  }
  FILE *f2 = fopen(file2, "r");
  if (!f2)
  {
    printf("Failed to open %s: %s\n", file2, strerror(errno));
    exit(1);
  }
  FILE *f3 = fopen(file3, "r");
  if (!f3)
  {
    printf("Failed to open %s: %s\n", file3, strerror(errno));
    exit(1);
  }

  unsigned char* buf1 = (unsigned char*)malloc(block_size);
  unsigned char* buf2 = (unsigned char*)malloc(block_size);
  unsigned char* buf3 = (unsigned char*)malloc(block_size);
  if (!buf1 || !buf2 || !buf3)
  {
    printf ("Allocation failure\n");
    if (buf1)
      free(buf1);
    if (buf2)
      free(buf2);
    if (buf3)
      free(buf3);
    exit(1);
  }

  fseek(f1, offset1, SEEK_SET);
  fseek(f2, offset2, SEEK_SET);
  fseek(f3, offset3, SEEK_SET);


  ssize_t overallpos = 0;
  while(1)
  {
    ssize_t amt1 = fread(buf1, 1, block_size, f1);
    ssize_t amt2 = fread(buf2, 1, block_size, f2);
    ssize_t amt3 = fread(buf3, 1, block_size, f3);
    
    if (amt2 > amt1)
      amt2 = amt1;
    if (amt1 > amt2)
      amt1 = amt2;

    if (amt3 > amt1)
      amt3 = amt1;
    if (amt3 < amt1)
      amt1 = amt2 = amt3;

    for ( ssize_t localpos = 0; localpos < amt1; ++localpos)
    {
      if (buf1[localpos] != buf2[localpos])
      {
        ssize_t actualpos = overallpos + localpos;
        ssize_t block_number = actualpos / block_size;
        int bits_differing = count_bits(buf1[localpos] ^ buf2[localpos]);
        int diff3v1 = count_bits(buf3[localpos] ^ buf1[localpos]);
        int diff3v2 = count_bits(buf3[localpos] ^ buf2[localpos]);
        printf("0x%010zx blk:%012zu 0x%02x 0x%02x 0x%02x difference: %d bits (vs 3rd file: %d%d)\n", actualpos, block_number, buf1[localpos], buf2[localpos], buf3[localpos], bits_differing, diff3v1, diff3v2);
      }
    }
    

    overallpos += amt1;
    if (amt1 != block_size)
    {
      printf("Compared 0x%zx bytes.\n", overallpos);
      break;
    }
  }

  
  free(buf1);
  free(buf2);
  free(buf3);
  exit(0);
}
