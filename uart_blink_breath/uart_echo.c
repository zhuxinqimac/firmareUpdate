#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/flash.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"

//****************************************************************************
//
// System clock rate in Hz.
//
//****************************************************************************
uint32_t g_ui32SysClock, times = 1;

//****************************************************************************
//
// Instructions for breathing LEDs.
//
//****************************************************************************
uint32_t i0 = 26, i1	= 39, i2 = 2, i3 = 13;
uint32_t j0 = 0, j1 = 0, j2 = 0, j3 = 0, k = 0;
bool flag0 = 1, flag1 = 1, flag2 = 1, flag3 = 1;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//****************************************************************************
//
// Input command.
//
//****************************************************************************
char command_char[100];
//****************************************************************************
//
// Jump to boot loader.
//
//****************************************************************************
void
JumpToBootLoader(void)
{
	
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
		GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
    //
    // We must make sure we turn off SysTick and its interrupt before entering 
    // the boot loader!
    //
    ROM_SysTickIntDisable(); 
    ROM_SysTickDisable(); 

    //
    // Disable all processor interrupts.  Instead of disabling them
    // one at a time, a direct write to NVIC is done to disable all
    // peripheral interrupts.
    //
    HWREG(NVIC_DIS0) = 0xffffffff;
    HWREG(NVIC_DIS1) = 0xffffffff;
    HWREG(NVIC_DIS2) = 0xffffffff;
    HWREG(NVIC_DIS3) = 0xffffffff;

    //
    // Return control to the boot loader.  This is a call to the SVC
    // handler in the boot loader.
    //
    (*((void (*)(void))(*(uint32_t *)0x2c)))(); 
}

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    uint32_t ui32Status, i;

    //
    // Get the interrrupt status.
    //
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    //
    // Clear the asserted interrupts.
    //
    ROM_UARTIntClear(UART0_BASE, ui32Status);
		for (i = 0; i<99; i++)
			command_char[i] = '\0';
	  

    //
    // Loop while there are characters in the receive FIFO.
    //
	  i = 0;
	  while(ROM_UARTCharsAvail(UART0_BASE))
    {
        //
        // Read the next character from the UART and write it back to the UART.
        //
			  command_char[i] = ROM_UARTCharGetNonBlocking(UART0_BASE);
			  command_char[i+1] = '\0';
			  if (command_char[i] == '#') break;
			  i++;

        //
        // Blink the LED to show a character transfer is occuring.
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);

        //
        // Delay for 1 millisecond.  Each SysCtlDelay is about 3 clocks.
        //
        SysCtlDelay(g_ui32SysClock / (1000 * 3));

        //
        // Turn off the LED
        //
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
    }
		// Flush.
		while(ROM_UARTCharsAvail(UART0_BASE))
    {
				ROM_UARTCharGetNonBlocking(UART0_BASE);
		}
		
		// Change mode.
		if (0 == strcmp(command_char, "READY#"))
		{
			  JumpToBootLoader();
		}
		else
		if (0 == strcmp(command_char, "R#"))
		{
				i0 = 26; i1	= 39; i2 = 2; i3 = 13;
				j0 = 0; j1 = 0; j2 = 0; j3 = 0; k = 0;
				flag0 = 1; flag1 = 1; flag2 = 1; flag3 = 1;
		}
		else
		if (0 == strcmp(command_char, "L#"))
		{
				i0 = 13; i1	= 2; i2 = 39; i3 = 26;
				j0 = 0; j1 = 0; j2 = 0; j3 = 0; k = 0;
				flag0 = 1; flag1 = 1; flag2 = 1; flag3 = 1;
		}
		else
		{
			// Times the speed of showing.
			uint32_t pos = 0, temp_times = 0;
			while (command_char[pos]!='#')
			{
				temp_times *= 10;
				temp_times += (command_char[pos]-48);
				pos++;
			}
			times = temp_times;
		}
}


//*****************************************************************************
//
// The main function.
// Show the breathing LEDs.
//
//*****************************************************************************
int
main(void)
{
    //
    // Set the clocking to run directly from the crystal at 120MHz.
    //
    g_ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_480), 120000000);
    //
    // Enable the GPIO port that is used for the on-board LED.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    //
    // Enable the GPIO pins for the LED (PN0).
    //
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, 
																GPIO_PIN_0|
																GPIO_PIN_1);
		ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, 
																GPIO_PIN_0|
																GPIO_PIN_4);
    //
    // Enable the peripherals used by this example.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //
    // Enable processor interrupts.
    //
    ROM_IntMasterEnable();

    //
    // Set GPIO A0 and A1 as UART pins.
    //
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);

    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Configure the UART for 115,200, 8-N-1 operation.
    //
    ROM_UARTConfigSetExpClk(UART0_BASE, g_ui32SysClock, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    //
    // Enable the UART interrupt.
    //
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

    //
    // Loop forever echoing data through the UART.
    //
    while(1)
    {
				if (k==0)
				{
						j0++;
						if (j0>=5)
						{
								j0 = 0;
								if (flag0)
									i0++;
								else
									i0--;
						}
						GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*i0);
						GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*(55-i0));
						if (i0>=54)
						{
							flag0 = 0;
						}
						if (i0<=2)
						{
							flag0 = 1;
						}
				}
				
				
				if (k==1)
				{
						j1++;
						if (j1>=5)
						{
								j1 = 0;
								if (flag1)
									i1++;
								else
									i1--;
						}
						GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
						SysCtlDelay(g_ui32SysClock/(100000*times)*i1);
						GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*(55-i1));
						if (i1>=54)
						{
							flag1 = 0;
						}
						if (i1<=2)
						{
							flag1 = 1;
						}
				}
				
				
				if (k==2)
				{
						j2++;
						if (j2>=5)
						{
								j2 = 0;
								if (flag2)
									i2++;
								else
									i2--;
						}
						GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*i2);
						GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*(55-i2));
						if (i2>=54)
						{
							flag2 = 0;
						}
						if (i2<=2)
						{
							flag2 = 1;
						}
				}
				
				
				if (k==3)
				{
						j3++;
						if (j3>=5)
						{
								j3 = 0;
								if (flag3)
									i3++;
								else
									i3--;
						}
						GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
						SysCtlDelay(g_ui32SysClock/(100000*times)*i3);
						GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
						SysCtlDelay(g_ui32SysClock/(100000*times)*(55-i3));
						if (i3>=54)
						{
							flag3 = 0;
						}
						if (i3<=2)
						{
							flag3 = 1;
						}
        }
        
				k++;
				if (k>3) k = 0;
    }
}
