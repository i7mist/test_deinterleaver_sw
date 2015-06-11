#include <stdio.h>
#include "sPcie.h"

#define SRCACK 4
#define DSTACK 2
#define FINISH 1

#define SRCRDY 4
#define DSTRDY 2
#define FINISH_ACK 1

#define BPSK_RATE 0xd
#define QPSK_RATE 0x5
#define QAM16_RATE 0x9
#define QAM64_RATE 0x1

#define REG_ADDR_SRC_TOTAL 2
#define REG_ADDR_DST_TOTAL 3
#define REG_ADDR_RATE 4
#define REG_ADDR_H2BSIGNALS 1
#define REG_ADDR_B2HSIGNALS 0

#define DMA_SIZE 10

int main()
{
	char ip_mode;
	unsigned int src_total, drain_total, rate; // reg 2, 3, 4
  unsigned int h2bsignals; // reg 1
  unsigned int b2hsignals; // reg 0
  unsigned char targetBuf[DMA_SIZE];
  unsigned char sourceBuf[DMA_SIZE];
  
	ip_mode = get_pcie_cfg_mode();
	printf(">configure mode:x%dgen%d\t", (ip_mode>>4)&0x0F, (ip_mode&0x0F));
	ip_mode = get_pcie_cur_mode();
	printf(" current mode:x%dgen%d\n", (ip_mode>>4)&0x0F, (ip_mode&0x0F));
	
  
  while((fread(sourceBuf, 1, DMA_SIZE, stdin) > 0)) {
    rate = BPSK_RATE;
    write_usr_reg(REG_ADDR_RATE, &rate);

    src_total = DMA_SIZE;
    drain_total = DMA_SIZE;
    h2bsignals = SRCRDY | DSTRDY;
    
    write_usr_reg(REG_ADDR_SRC_TOTAL, &src_total);
    write_usr_reg(REG_ADDR_DST_TOTAL, &drain_total);
    write_usr_reg(REG_ADDR_H2BSIGNALS, &h2bsignals);
    while(1) {
      read_usr_reg(REG_ADDR_B2HSIGNALS, &b2hsignals);
      if ((b2hsignals & SRCACK)) {
        h2bsignals &= (~SRCRDY);
      }
      if ((b2hsignals & DSTACK)) {
        h2bsignals &= (~DSTRDY);
      }
      if ((!(h2bsignals & SRCRDY)) && (!(h2bsignals & DSTRDY))) {
        write_usr_reg(REG_ADDR_H2BSIGNALS, &h2bsignals);
        break;
      }
    }
    size_t write_size = 0;
    size_t size;
    while(1) {
      while ((size = dma_host2board_unblocking(DMA_SIZE,
              (unsigned char*)sourceBuf)) < 0) {
        printf("dma_host2board busy!\n");
      }
      if ((size = dma_board2host(DMA_SIZE,
           (unsigned char*)targetBuf)) < 0) {
        printf("dma_board2host error!\n");
        return 0;
      }
      write_size += size;
      fwrite(targetBuf, 1, size, stdout);
      read_usr_reg(REG_ADDR_B2HSIGNALS, &b2hsignals);
      if (b2hsignals & FINISH) {
        h2bsignals = FINISH_ACK;
        write_usr_reg(REG_ADDR_H2BSIGNALS, &h2bsignals);
        while (write_size < DMA_SIZE) {
          if ((size = dma_board2host(DMA_SIZE,
               (unsigned char*)targetBuf)) < 0) {
            printf("dma_board2host error!\n");
            return 0;
          }
          write_size += size;
        }
        break;
      }
    }
  }
	return 0;
}
