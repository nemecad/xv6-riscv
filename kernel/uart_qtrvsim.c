//
// low-level driver for 16550a UART.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "uart_qtrvsim.h"

// the UART control registers are memory-mapped
// at address UART0. this macro returns the
// address of one of the registers.
#define Reg(reg) ((volatile unsigned char *)(SERIAL_PORT_BASE + (reg)))

#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// for sending threads to synchronize with uart "ready" interrupts.
static struct spinlock tx_lock;
static int tx_busy;           // is the UART busy sending?
static int tx_chan;           // &tx_chan is the "wait channel"

extern volatile int panicking; // from printf.c
extern volatile int panicked; // from printf.c

void
uartinit(void)
{
  // disable interrupts.
  WriteReg(SERP_TX_ST_REG_o, 0x00);
  WriteReg(SERP_RX_ST_REG_o, 0x00);

  // special mode to set baud rate.
  // not applicable for QtRvSim UART

  // LSB for baud rate of 38.4K.
  // not applicable for QtRvSim UART

  // MSB for baud rate of 38.4K.
  // not applicable for QtRvSim UART

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  // not applicable for QtRvSim UART

  // reset and enable FIFOs.
  // not applicable for QtRvSim UART

  // enable transmit and receive interrupts.
  //WriteReg(SERP_TX_ST_REG_o, SERP_TX_ST_REG_IE_m);
  WriteReg(SERP_RX_ST_REG_o, SERP_RX_ST_REG_IE_m);

  initlock(&tx_lock, "uart");
}

// transmit buf[] to the uart. it blocks if the
// uart is busy, so it cannot be called from
// interrupts, only from write() system calls.
void
uartwrite(char buf[], int n)
{
  acquire(&tx_lock);

  int i = 0;
  while(i < n){
   #if 0 
    while(tx_busy != 0){
      // wait for a UART transmit-complete interrupt
      // to set tx_busy to 0.
      sleep(&tx_chan, &tx_lock);
    }
   #endif
    WriteReg(SERP_TX_DATA_REG_o, buf[i]);
    i += 1;
    tx_busy = 1;
  }

  release(&tx_lock);
}


// write a byte to the uart without using
// interrupts, for use by kernel printf() and
// to echo characters. it spins waiting for the uart's
// output register to be empty.
void
uartputc_sync(int c)
{
  if(panicking == 0)
    push_off();

  if(panicked){
    for(;;)
      ;
  }

  // wait for UART to set Transmit Holding Empty in LSR.
  while((ReadReg(SERP_TX_ST_REG_o) & SERP_TX_ST_REG_READY_m) == 0)
    ;
  WriteReg(SERP_TX_DATA_REG_o, c);

  if(panicking == 0)
    pop_off();
}

// try to read one input character from the UART.
// return -1 if none is waiting.
int
uartgetc(void)
{
  if(ReadReg(SERP_RX_ST_REG_o) & SERP_RX_ST_REG_READY_m){
    // input data is ready.
    return ReadReg(SERP_RX_DATA_REG_o);
  } else {
    return -1;
  }
}

// handle a uart interrupt, raised because input has
// arrived, or the uart is ready for more output, or
// both. called from devintr().
void
uartintr(void)
{
  acquire(&tx_lock);
  if(ReadReg(SERP_TX_ST_REG_o) & SERP_TX_ST_REG_READY_m){
    WriteReg(SERP_TX_ST_REG_o, SERP_TX_ST_REG_IE_m * 0);
    // UART finished transmitting; wake up sending thread.
    tx_busy = 0;
    wakeup(&tx_chan);
  }
  release(&tx_lock);

  // read and process incoming characters, if any.
  while(1){
    int c = uartgetc();
    if(c == -1)
      break;
    consoleintr(c);
  }
}
