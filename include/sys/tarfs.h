#ifndef _TARFS_H
#define _TARFS_H

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;

void init_tarfs();

typedef struct posix_header_ustar posix_header_ustar;

struct posix_header_ustar {
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char checksum[8];
  char typeflag[1];
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
  char pad[12];
};

typedef struct tarfsinfo tarfsinfo;

struct tarfsinfo
{
  uint8_t* fname;
  uint32_t fsize;
  uint64_t fstartaddr;
};

uint64_t _tarfsopen(uint8_t *);

uint64_t _tarfs_read(uint64_t start_add, uint8_t *buf, uint32_t size);

#endif
