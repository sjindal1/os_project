#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/ahci.h>

#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define AHCI_DEV_NULL 0
#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier
 
uint16_t pciConfigReadWord (uint8_t bus, uint8_t slot,
                             uint8_t func, uint8_t offset);
int checkDevice(uint8_t bus, uint8_t device, uint8_t f);
void checkAllBuses(void);
uint16_t getVendorID(uint8_t bus, uint8_t device,uint8_t function);
static inline void sysOutLong(uint16_t config_addr, uint32_t value);
static inline uint32_t sysInLong(uint16_t config_addr);
uint8_t getHeaderType(uint8_t bus,uint8_t device,uint8_t function);
uint16_t getClassInfo(uint8_t bus,uint8_t device,uint8_t function);
void parse_ahci(uint8_t bus,uint8_t device,uint8_t function);
static int check_type(hba_port_t *port);
void probe_port(hba_mem_t *abar);

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

int checkDevice(uint8_t bus, uint8_t device, uint8_t f) {
     uint8_t function = f;
     uint16_t vendorID = getVendorID(bus, device, function);
     
     if(vendorID == 0xFFFF) return 0;        // Device doesn't exist
     if(((pciConfigReadWord( bus, device, function, 0x0A ) >> 8)& 0xff) ==1 && 
        (pciConfigReadWord( bus, device, function, 0x0A )& 0xff) == 6){
       kprintf("AHCI found vendor id : %x %x\n", vendorID, pciConfigReadWord( bus, device, function, 0x2 ));       
       parse_ahci(bus, device, function);
       return 0;
   }
   return 0;
 }

void checkAllBuses(void) {
  uint16_t bus;
  uint8_t device;
  uint8_t f;
  for(bus = 0; bus < 256; bus++) {
   for(device = 0; device < 32; device++) {
     for(f = 0; f < 8; f++){
       if(checkDevice((uint8_t)bus, device, f) == 1) return;
     }
   }
 }
}


void parse_ahci(uint8_t bus, uint8_t device, uint8_t function) {
  uint64_t abar;
  uint16_t bar5_l = pciConfigReadWord( bus, device, function, 0x24 );
  sysOutLong (0xCFC, 0x20000000); //move the ahci to a valid memory address
  bar5_l = pciConfigReadWord( bus, device, function, 0x24 );
  uint16_t bar5_h = pciConfigReadWord( bus, device, function, 0x26 );
  uint32_t abar5 =  (uint32_t)bar5_h << 16|bar5_l ; 
  abar = (uint64_t)abar5;
  kprintf("abar %x\n", abar);
  probe_port((hba_mem_t *)abar); 
}

void dump_abar(hba_mem_t *abar){
  kprintf("cap -> %x\n", abar->cap);
  kprintf("ghc -> %x\n", abar->ghc);
  kprintf("is_rwc -> %x\n", abar->is_rwc);
  kprintf("pi -> %x\n", abar->pi);
  kprintf("vs -> %x\n", abar->vs);
  kprintf("ccc_ctl -> %x\n", abar->ccc_ctl);
  kprintf("ccc_pts -> %x\n", abar->ccc_pts); 
  kprintf("em_loc -> %x\n", abar->em_loc);
  kprintf("em_ctl -> %x\n", abar->em_ctl);
  kprintf("cap2 -> %x\n", abar->cap2);
  kprintf("bohc -> %x\n", abar->bohc);
}

void probe_port(hba_mem_t *abar)
{
        // Search disk in impelemented ports
	uint32_t pi = abar->pi;
//        dump_abar(abar);
        int i = 0;
	while (i<32)
	{
		if (pi & 1)
		{
			int dt = check_type(&abar->ports[i]);
			if (dt == AHCI_DEV_SATA)
			{
				kprintf("SATA drive found at port %d\n", i);
			}
			else if (dt == AHCI_DEV_SATAPI)
			{
				kprintf("SATAPI drive found at port %d\n", i);
			}
			else if (dt == AHCI_DEV_SEMB)
			{
				kprintf("SEMB drive found at port %d\n", i);
			}
			else if (dt == AHCI_DEV_PM)
			{
				kprintf("PM drive found at port %d\n", i);
			}
			else
			{
			        kprintf("No drive found at port %d\n", i);
			}
		}
 
		pi >>= 1;
		i ++;
	}
}
 
// Check device type
static int check_type(hba_port_t *port)
{
	uint32_t ssts = port->ssts;
 
	uint8_t ipm = (ssts >> 8) & 0x0F;
	uint8_t det = ssts & 0x0F;
 
	if (det != HBA_PORT_DET_PRESENT)	// Check drive status
		return AHCI_DEV_NULL;
	if (ipm != HBA_PORT_IPM_ACTIVE)
		return AHCI_DEV_NULL;
 
	switch (port->sig)
	{
	case SATA_SIG_ATAPI:
		return AHCI_DEV_SATAPI;
	case SATA_SIG_SEMB:
		return AHCI_DEV_SEMB;
	case SATA_SIG_PM:
		return AHCI_DEV_PM;
	default:
		return AHCI_DEV_SATA;
	}
}
