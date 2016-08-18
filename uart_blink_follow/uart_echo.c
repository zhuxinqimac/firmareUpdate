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
uint32_t g_ui32SysClock;
bool b1 = 0, b2 = 0, b3 = 0, b4 = 0;

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


char command_char[100];

//*****************************************************************************
//
// Jump to boot loader.
//
//*****************************************************************************
void
JumpToBootLoader(void)
{
	  //To show it is in boot loader mode.
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
		while(ROM_UARTCharsAvail(UART0_BASE))
    {
				ROM_UARTCharGetNonBlocking(UART0_BASE);
		}
		
		if (0 == strcmp(command_char, "READY#"))
		{
			  JumpToBootLoader();
		}
		else
		if (0 == strcmp(command_char, "1#"))
		{
			  if (b1) b1 = 0;
			  else b1 = 1;
		}
		if (0 == strcmp(command_char, "2#"))
		{
			  if (b2) b2 = 0;
			  else b2 = 1;
		}
		if (0 == strcmp(command_char, "3#"))
		{
			  if (b3) b3 = 0;
			  else b3 = 1;
		}
		if (0 == strcmp(command_char, "4#"))
		{
			  if (b4) b4 = 0;
			  else b4 = 1;
		}
}


//*****************************************************************************
//
// The main function.
// Let the LEDs follow phone's screen.
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
    // Enable the GPIO pins for the LEDs.
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

    // Loop forever echoing data through the UART.
    //
    while(1)
    {
				if (b1)
					GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1);
				else
					GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);
        
				if (b2)
					GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, GPIO_PIN_0);
				else
					GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, 0);
				
				if (b3)
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_PIN_4);
				else
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_4, 0);
				
				if (b4)
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_PIN_0);
				else
					GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
    }
}
