//***************************************************
//code reference OSDev PCI config page and AHCI page
//link - wiki.osdev.org/ahci, wiki.osdev.org/pci
//***************************************************
#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/ahci.h>
#include <sys/utils.h>

#define HBA_PORT_DET_PRESENT 3
#define HBA_PORT_IPM_ACTIVE 1
#define AHCI_DEV_NULL 0
#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier
#define hba_port_cmd_cr (1 << 15)
#define hba_port_cmd_fre (1 << 4)
#define hba_port_cmd_fr (1 << 14)
#define hba_port_cmd_sud (1 << 1)
#define hba_port_cmd_start 1
#define AHCI_ADDR 0x800000

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
uint16_t write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint64_t *buf);
uint16_t read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint64_t *buf);

void sleep(){
  for(volatile uint64_t i=5000000;i>0;i--)
   i=i+0;
}

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
        kprintf("AHCI found vendor id : %x %x bus - %d device - %d function - %d\n", vendorID, pciConfigReadWord( bus, device, function, 0x2 ), bus, device, function);       
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

void reset_hba(hba_mem_t *abar){
  //abar->ghc = 1;
  //abar->ghc|= 0x2;
  abar->ghc |= 0x80000000;
  sleep();
  sleep();
  sleep();
  /*abar->ghc |= 0x1;
  while(abar->ghc & 1)
    sleep();
  sleep();
  if(!(abar->ghc & 0x80000000))
    abar->ghc |= 0x80000000;*/
}

void parse_ahci(uint8_t bus, uint8_t device, uint8_t function) {
  volatile uint64_t abar;
  uint16_t bar5_l = pciConfigReadWord( bus, device, function, 0x24 );
  sysOutLong (0xCFC, 0xB0000); //move the ahci to a valid memory address
  bar5_l = pciConfigReadWord( bus, device, function, 0x24 );
  uint16_t bar5_h = pciConfigReadWord( bus, device, function, 0x26 );
  uint32_t abar5 =  (uint32_t)bar5_h << 16|bar5_l ; 
  abar = (uint64_t)abar5;
  //kprintf("abar %x\n", abar);
  reset_hba((hba_mem_t *)abar);
  sleep();
  probe_port((hba_mem_t *)abar); 
}


int wake_sata(hba_port_t *port){
  
  //volatile uint32_t sctl = port->sctl;
  //volatile uint32_t sctl_det = (sctl & 0xfffff0f0)|0x301;
  //port->sctl = sctl_det;
  port->sctl = 0x301;
  sleep();

  //sctl = port->sctl;
  //volatile uint32_t sctl_ipm = (sctl & 0xfffff0f0) | 0x300;
  //port->sctl = sctl_ipm;
  port->sctl = 0x300;
  sleep();

  port->cmd &= 0x0fffffff;
  port->cmd |= (0x10000000 | 0x6 );
  sleep();
  port->serr_rwc = 0xffffffff;
  port->is_rwc = 0xffffffff;
  uint64_t i = 50000000;
  while((port->ssts|0xF) !=0x3 ){
    ;
    if(--i == 0){
      break;
    }
  }
  kprintf("wake sata port - ssts = %x sctl = %x \n", port->ssts, port->sctl);
  kprintf("wake sata port - sig = %x \n", port->sig);
  return 0;
}

// Stop command engine
void stop_cmd(hba_port_t *port)
{
  // Clear ST (bit0)
  port->cmd &= 0xfffffffe;
 
  // Wait until FR (bit14), CR (bit15) are cleared
  while(1)
  {
    if (port->cmd & hba_port_cmd_fr)
      continue;
    if (port->cmd & hba_port_cmd_cr)
      continue;
    break;
  }
 
  // Clear FRE (bit4)
  port->cmd &= 0xffffffef;
}

void start_cmd(hba_port_t *p1){
  while(p1->cmd & hba_port_cmd_cr);

  p1->cmd |= hba_port_cmd_fre;
  p1->cmd |= hba_port_cmd_start; 
}

void port_rebase(hba_port_t *port, int port_num){
  stop_cmd(port);
  hba_port_t *p1 = port;

  // Command list offset: 1K*portno
  // Command list entry size = 32
  // Command list entry maxim count = 32
  // Command list maxim size = 32*32 = 1K per port
  p1->clb = AHCI_ADDR + (port_num<<10);
  memset((void*)(p1->clb), 0, 1024);
 
  // FIS offset: 32K+256*portno
  // FIS entry size = 256 bytes per port
  p1->fb = AHCI_ADDR + (32<<10) + (port_num<<8);
  memset((void*)(p1->fb), 0, 256);

  // Command table offset: 40K + 8K*portno
  // Command table size = 256*32 = 8K per port
  hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)(p1->clb);
  for (int i=0; i<32; i++)
  {
    cmdheader[i].prdtl = 8;	// 8 prdt entries per command table
				// 256 bytes per command table, 64+16+48+16*8
    // Command table offset: 40K + 8K*portno + cmdheader_index*256
    cmdheader[i].ctba = AHCI_ADDR + (40<<10) + (port_num<<13) + (i<<8);
    memset((void*)cmdheader[i].ctba, 0, 256);
  }
  //start_cmd(port);
}

void reset_port(hba_mem_t *abar, int32_t port_num){
  hba_port_t *port = &abar->ports[port_num];
  //If port is not idle force it to be idle
  if(port->cmd & (HBA_PxCMD_ST | HBA_PxCMD_CR | HBA_PxCMD_FRE | HBA_PxCMD_FR))
    stop_cmd(port);
  port_rebase(port, port_num);
  wake_sata(port);
  start_cmd(port);
  return;
}

void read_write_data2hdd(hba_mem_t *abar){
  //reset_hba(abar);
  reset_port(abar, 0);
  uint64_t *buf = (uint64_t *)0x400000;
  uint64_t *buf2 = (uint64_t *)0x500000;
  for(int i=0;i<100;i++){
    memset(buf, i, 4096);
    buf2[0] = 0xdeaddeaddeaddead;
    buf2[1] = 0xdeaddeaddeaddead;
    write(&abar->ports[0],i*8,0,8,buf);
    sleep();
    //kprintf("ssts -> %x is_rwc -> %x tfd -> %x\n", abar->ports[0].ssts, abar->ports[0].is_rwc, abar->ports[0].tfd);
    read(&abar->ports[0],i*8,0,8,buf2);
    sleep();
    kprintf("readb->%d ",(uint8_t) buf2[0]);
  } 
}

void probe_port(hba_mem_t *abar)
{
  volatile uint32_t pi = abar->pi;
  int i = 0;
  //kprintf("pi = %x\n", pi);
  //kprintf("cap = %x\n", abar->cap);
  while (i<32)
  {
    if (pi & 1)
    {
      int dt = check_type(&abar->ports[i]);
      if (dt == AHCI_DEV_SATA)
      {
        kprintf("SATA drive found at port %d\n", i);
	/*p = &abar->ports[i];
        kprintf(" port %d clb = %p fb = %p cmd = %x\n", i, p->clb, p->fb, p->cmd);*/       }
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
        //kprintf("No drive found at port %d\n", i);
      }
    }
    pi >>= 1;
    i ++;
  }
  read_write_data2hdd(abar);
}
 
// Check device type
static int check_type(hba_port_t *port)
{
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

int32_t find_cmdslot(hba_port_t*);

#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_WRITE_DMA_EX 0x35


uint16_t read(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint64_t *buf)
{
  int i;
  port->is_rwc = 0xffffffff;    // Clear pending interrupt bits
  uint32_t spin = 0; // Spin lock timeout counter
  int slot = find_cmdslot(port);
  if (slot == -1)
    return 0;
 
  hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)port->clb;
  cmdheader += slot;
  cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);  // Command FIS size
  cmdheader->w = 0;    // Read from device
  cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;  // PRDT entries count
 
  hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
  memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) +
     (cmdheader->prdtl-1)*sizeof(hba_prdt_entry_t));
 
  // 8K bytes (16 sectors) per PRDT
  for (i=0; i<cmdheader->prdtl-1; i++)
  {
    cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
    cmdtbl->prdt_entry[i].dbc = 8*1024;  // 8K bytes
    cmdtbl->prdt_entry[i].i = 1;
    buf += 4*1024;  // 4K words
    count -= 16;  // 16 sectors
  }
  // Last entry
  cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
  cmdtbl->prdt_entry[i].dbc = count<<9;  // 512 bytes per sector
  cmdtbl->prdt_entry[i].i = 1;
 
  // Setup command
  fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;  // Command
  cmdfis->command = ATA_CMD_READ_DMA_EX;
 
  cmdfis->lba0 = (uint8_t)startl;
  cmdfis->lba1 = (uint8_t)(startl>>8);
  cmdfis->lba2 = (uint8_t)(startl>>16);
  cmdfis->device = 1<<6;  // LBA mode
 
  cmdfis->lba3 = (uint8_t)(startl>>24);
  cmdfis->lba4 = (uint8_t)starth;
  cmdfis->lba5 = (uint8_t)(starth>>8);
 
  cmdfis->count = count;// & 0xffff;
  //cmdfis->counth = (count >> 16) & 0xffff;
 
  // The below loop waits until the port is no longer busy before issuing a new command
  while ((port->tfd & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) && spin < 1000000)
  {
    spin++;
  }
  if (spin == 1000000)
  {
    kprintf("Port is hung\n");
    return 0;
  }
 
  port->ci = 1 << slot;  // Issue command
 
  // Wait for completion
  while (1)
  {
    // In some longer duration reads, it may be helpful to spin on the DPS bit 
    // in the PxIS port field as well (1 << 5)
    if ((port->ci & (1 << slot)) == 0) 
      break;
    if (port->is_rwc & HBA_PxIS_TFES)  // Task file error
    {
      kprintf("1 -- Read disk error\n");
      return 0;
    }
  }
 
  // Check again
  if (port->is_rwc & HBA_PxIS_TFES)
  {
    kprintf("2 -- Read disk error\n");
    return 0;
  }
 
  return 1;
}

uint16_t write(hba_port_t *port, uint32_t startl, uint32_t starth, uint32_t count, uint64_t *buf)
{
  int i;
  port->is_rwc = 0xffffffff;    // Clear pending interrupt bits
  uint64_t spin = 0; // Spin lock timeout counter
  int slot = find_cmdslot(port);
  if (slot == -1)
    return 0;
 
  hba_cmd_header_t *cmdheader = (hba_cmd_header_t*)port->clb;
  cmdheader += slot;
  cmdheader->cfl = sizeof(fis_reg_h2d_t)/sizeof(uint32_t);  // Command FIS size
  cmdheader->w = 1;    // write to device
  //cmdheader->c = 1;
  //cmdheader->p = 1;
  cmdheader->prdtl = (uint16_t)((count-1)>>4) + 1;  // PRDT entries count
 
  hba_cmd_tbl_t *cmdtbl = (hba_cmd_tbl_t*)(cmdheader->ctba);
  memset(cmdtbl, 0, sizeof(hba_cmd_tbl_t) +
     (cmdheader->prdtl-1)*sizeof(hba_prdt_entry_t));
 
  // 8K bytes (16 sectors) per PRDT
  for (i=0; i<cmdheader->prdtl-1; i++)
  {
    cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
    cmdtbl->prdt_entry[i].dbc = 8*1024;  // 8K bytes
    cmdtbl->prdt_entry[i].i = 1;
    buf += 4*1024;  // 4K words
    count -= 16;  // 16 sectors
  }
  // Last entry
  cmdtbl->prdt_entry[i].dba = (uint64_t)buf;
  cmdtbl->prdt_entry[i].dbc = count<<9;  // 512 bytes per sector
  cmdtbl->prdt_entry[i].i = 1;
 
  // Setup command
  fis_reg_h2d_t *cmdfis = (fis_reg_h2d_t*)(&cmdtbl->cfis);
 
  cmdfis->fis_type = FIS_TYPE_REG_H2D;
  cmdfis->c = 1;  // Command
  cmdfis->command = ATA_CMD_WRITE_DMA_EX;
 
  cmdfis->lba0 = (uint8_t)startl;
  cmdfis->lba1 = (uint8_t)(startl>>8);
  cmdfis->lba2 = (uint8_t)(startl>>16);
  cmdfis->device = 1<<6;  // LBA mode
 
  cmdfis->lba3 = (uint8_t)(startl>>24);
  cmdfis->lba4 = (uint8_t)starth;
  cmdfis->lba5 = (uint8_t)(starth>>8);
 
  cmdfis->count = count;// & 0xffff;
  //cmdfis->counth = (count >> 16) & 0xffff;
 
  // The below loop waits until the port is no longer busy before issuing a new command
  //kprintf("here 1");
  while ((port->tfd & (ATA_STATUS_BSY | ATA_STATUS_DRQ)) && spin < 1000000)
  {
    spin++;
  }
  if (spin == 1000000)
  {
    kprintf("Port is hung\n");
    return 0;
  }
  //kprintf("here 2");
  port->ci = 1 << slot;  // Issue command
 
  // Wait for completion
  while (1)
  {
    // In some longer duration reads, it may be helpful to spin on the DPS bit 
    // in the PxIS port field as well (1 << 5)
    if ((port->ci & (1 << slot)) == 0) 
      break;
    if (port->is_rwc & HBA_PxIS_TFES)  // Task file error
    {
      kprintf("1 --- Read disk error\n");
      return 0;
    }
  }
  //kprintf("here 3");
  // Check again
  if (port->is_rwc & HBA_PxIS_TFES)
  {
    kprintf("2 -- Read disk error\n");
    return 0;
  }
 
  return 1;
}

//pru temp
//uint16_t cmdslots = 32;

// Find a free command list slot
int32_t find_cmdslot(hba_port_t *port)
{
  // If not set in SACT and CI, the slot is free
  uint32_t slots = (port->sact | port->ci);
  for (int i = 0; i < MAX_CMD_SLOT_CNT; i++)
  {
    if ((slots & 1) == 0)
      return i;
    slots >>= 1;
  }
  kprintf("Cannot find free command list entry\n");
  return -1;
}


