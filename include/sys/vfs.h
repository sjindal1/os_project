
// vfs. h
// header file apis of VFS layer which interact with
//1. Terminal
//2. Tarfs
//3. Filesystem
//4. Network
// through FDs.

int16_t _vfsopen(uint8_t* filename);

uint32_t _vfsread(int16_t filedescriptor, uint8_t* buffer, uint16_t size);

uint32_t _vfswrite(int16_t filedescriptor, uint8_t* buffer, uint16_t size);

void _vfsseek(int16_t fd, uint32_t offset);

