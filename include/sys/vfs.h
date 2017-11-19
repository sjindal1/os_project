
// vfs. h
// header file apis of VFS layer which interact with
//1. Terminal
//2. Tarfs
//3. Filesystem
//4. Network
// through FDs.

uint64_t _vfsopen(uint8_t* filename);

uint32_t _vfsread(uint16_t filedescriptor, uint8_t* buffer, uint16_t size);

uint32_t _vfswrite(uint16_t filedescriptor, uint8_t* buffer, uint16_t size);

