#include <sys/defs.h>
#include <sys/kprintf.h>

uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                             uint8_t func, uint8_t offset);
void checkDevice(uint8_t bus, uint8_t device);
void checkAllBuses(void);
uint16_t getVendorID(uint8_t bus, uint8_t device,uint8_t function);
static inline void sysOutLong(uint16_t config_addr, uint32_t value);
void checkFunction(uint8_t bus, uint8_t device, uint8_t function);
static inline uint32_t sysInLong(uint16_t config_addr);
uint8_t getHeaderType(uint8_t bus,uint8_t device,uint8_t function);
uint16_t getClassInfo(uint8_t bus,uint8_t device,uint8_t function);

static inline void sysOutLong(uint16_t config_addr, uint32_t value){
  __asm__ __volatile__("outl %0, %1\n\t"
                       :
                       :"a"(value),"Nd"(config_addr));
  
}

static inline uint32_t sysInLong(uint16_t config_addr){
  uint32_t value;
  __asm__ __volatile__("inl %1, %0\n\t"
                       :"=a"(value)
                       :"Nd"(config_addr));
  return (uint32_t)value;
}

uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                            uint8_t func, uint8_t offset)
{
   uint32_t address;
   uint32_t lbus  = (uint32_t)bus;
   uint32_t lslot = (uint32_t)slot;
   uint32_t lfunc = (uint32_t)func;
   uint16_t tmp = 0;

   /* create configuration address as per Figure 1 */
   address = (uint32_t)((lbus << 16) | (lslot << 11) |(lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));

   /* write out the address */
   sysOutLong (0xCF8, address);
   /* read in the data */
   /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
   tmp = (uint16_t)((sysInLong (0xCFC) >> ((offset & 2) * 8)) & 0xffff);
   return (tmp);
}

uint16_t getVendorID(uint8_t bus,uint8_t  device, uint8_t function){
  return pciConfigReadWord( bus, device, function, 0);
}
uint8_t getHeaderType(uint8_t bus, uint8_t device, uint8_t function){
  return (uint8_t) (pciConfigReadWord(bus, device, function, 0xE) & 0xF);
}

uint16_t getClassInfo(uint8_t bus,uint8_t  device, uint8_t function){
  return pciConfigReadWord( bus, device, function, 0xA );
}

void checkDevice(uint8_t bus, uint8_t device) {
     uint8_t function = 0;
     //kprintf("inside check device %d %d\n", bus, device);
     uint16_t vendorID = getVendorID(bus, device, function);
     //kprintf("inside check device %d %d %x\n", bus, device, vendorID);
     
     if(vendorID == 0xFFFF) return;        // Device doesn't exist
     checkFunction(bus, device, function);
     kprintf("vendor id = %x\n", vendorID);
     uint16_t prog_if = pciConfigReadWord(bus,device, function, 0x08);
     if(getClassInfo(bus, device, function) == 0x0106 && (prog_if >> 8) == 0x01){
       kprintf("AHCI found vendor id : %x\n", vendorID);
     }
     /*uint8_t headerType = getHeaderType(bus, device, function);
     if( (headerType & 0x80) != 0) {
          It is a multi-function device, so check remaining functions */
     /*    for(function = 1; function < 8; function++) {
             if(getVendorID(bus, device, function) != 0xFFFF) {
                 checkFunction(bus, device, function);
             }
         }
     }*/
 }

void checkAllBuses(void) {
  uint16_t bus;
  uint8_t device;
  for(bus = 0; bus < 256; bus++) {
   for(device = 0; device < 32; device++) {
       checkDevice((uint8_t)bus, device);
     }
   }
 }

void checkFunction(uint8_t bus, uint8_t device, uint8_t function) {
  //if(bus > 0)
    kprintf("device - %x, bus - %x, function - %x \n", device, bus, function); 
}
