//
// Modified from:
//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Access from ARM Running Linux
#define BCM2708_PERI_BASE        0x7E000000
#define ARM_PERI_BASE            0x20000000
#define GPIO_OFFSET              0x00200000
#define CMGP_OFFSET              0x00101000
#define GPIO_BASE                (ARM_PERI_BASE+GPIO_OFFSET) /* GPIO controller */
#define CMGP_BASE                (ARM_PERI_BASE+CMGP_OFFSET) /* CM controller */

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_REG(reg) (gpio[(reg-(BCM2708_PERI_BASE+GPIO_OFFSET))/4])
#define CMGP_REG(reg) (cmgp[(reg-(BCM2708_PERI_BASE+CMGP_OFFSET))/4])

#define GPFSEL0 (0x7E200000)
#define CM_GP0CTL (0x7E101070)
#define CM_GP0DIV (0x7E101074)

//
// Set up a memory regions to access GPIO
//
volatile unsigned int *setup_io(reg_base)
{
    int  mem_fd;
    void *io_map;

    // open /dev/mem
    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
    {
	printf("can't open /dev/mem \n");
	exit(-1);
    }
    // mmap IO
    io_map = mmap(NULL,             //Any adddress in our space will do
		  4096,             //Map length
		  PROT_READ|PROT_WRITE,// Enable reading & writting to mapped memory
		  MAP_SHARED,       //Shared with other processes
		  mem_fd,           //File to map
		  reg_base);        //Offset to peripheral
    close(mem_fd); //No need to keep mem_fd open after mmap
    if (io_map == MAP_FAILED)
    {
	printf("mmap error %d\n", (int)io_map);//errno also set!
	exit(-1);
    }
    return (volatile unsigned *)io_map;
 }

// I/O access
volatile unsigned *gpio, *cmgp;

int main(int argc, char **argv)
{
    int g,rep;

    // Set up gpi pointer for direct register access
    cmgp = setup_io(ARM_PERI_BASE + CMGP_OFFSET);
    gpio = setup_io(ARM_PERI_BASE + GPIO_OFFSET);

    // Set Clock Manager to provivede ~1.8432 MHz from 500 MHz source (PLLD)
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                               // Disable
	                | (1);         // Src = oscillator
    usleep(1000);
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                               // Disable
	                | (6);         // Src = PLLD
    CMGP_REG(CM_GP0DIV) = (0x5A << 24) // Password
                        | (271 << 12); // IDIV
    usleep(1000);
    CMGP_REG(CM_GP0CTL) = (0x5A << 24) // Password
	                | (1 << 4)     // Enable
	                | (6);         // Src = PLLD
    usleep(1000);
    // Set GCLK function (ALT0) for GPIO 4 (header pin #7)
    INP_GPIO(4);
    SET_GPIO_ALT(4, 0x00);

    return 0;
}

