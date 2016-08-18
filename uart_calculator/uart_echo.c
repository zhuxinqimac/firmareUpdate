#include <stdlib.h>
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

// Jump to boot loader
void
JumpToBootLoader(void)
{
		//ROM_UARTCharPutNonBlocking(UART0_BASE, 'b');
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
// Calculator
//*****************************************************************************
char command_char[100];

typedef struct stacknode
{
    char data;
    struct stacknode *next;
}StackNode, *StackNodePtr;

StackNode Stack_Ope[20];
StackNode Stack_Num[20];

typedef struct
{
    StackNodePtr top;
    int count;
}LinkStack;

void InitStack(LinkStack *S)
{
    S->top = NULL;
    S->count = 0;
}

char GetTop(LinkStack *S)
{
    if(!(S->top))
        return '\0';
    return S->top->data;
}

void Push(LinkStack *S, StackNode* DisStack, char e)
{
		StackNodePtr p = &DisStack[S->count];
		p->data = e;
		p->next = S->top;
    S->top = p;
    S->count++;  
}

char Pop(LinkStack *S)
{
    StackNodePtr p;
    char data;

    if(S->top == NULL)
    {
        return '\0';
    }

    p = S->top;
    S->top = p->next;
    S->count--;
    data = p->data;
		p->data = 0;
		p->next = NULL;

    return data;
}

int ChooseIJ(char index)
{
    int i;
    switch(index)
    {
    case '+':
        i=0;
        break;
    case '-':
        i=1;
        break;
    case '*':
        i=2;
        break;
    case '/':
        i=3;
        break;
    case '(':
        i=4;
        break;
    case ')':
        i=5;
        break;
    case '#':
        i=6;
    }

    return i;
}
  
char Judge(char top, char ch)
{
    static char order[][7]={
      // +   -   *   /   (   )   #
/* + */ '>','>','<','<','<','>','>',
/* - */ '>','>','<','<','<','>','>',
/* * */ '>','>','>','>','<','>','>',
/* / */ '>','>','>','>','<','>','>',
/* ( */ '<','<','<','<','<','=',' ',
/* ) */ '>','>','>','>',' ','>','>',
/* # */ '<','<','<','<','<',' ','='
    };
    int i,j;

    i = ChooseIJ(top);
    j = ChooseIJ(ch);
    return order[i][j];
}
  
int Operate(int O1, char op, int O2)
{
    int result;

    O1 -= 48;
    O2 -= 48;
  
    switch(op)
    {
    case '+':
        result = O1 + O2;
        break;
    case '-':
        result = O1 - O2;
        break;
    case '*':
        result = O1 * O2;
        break;
    case '/':
        result = O1 / O2;
        break;
    }

    return result+48;
}
 
char DoCalculate()
{
    LinkStack Ope, Num;
    char a,b,c,d,chOpe;
		uint32_t i;
		i = 0;
		while (command_char[i]!='#')
			ROM_UARTCharPutNonBlocking(UART0_BASE, command_char[i++]);
		ROM_UARTCharPutNonBlocking(UART0_BASE, '=');
		i = 0;
    InitStack(&Ope);
    Push(&Ope, Stack_Ope, '#');

    InitStack(&Num);
    c = command_char[i];
	  i++;

		while(c != '#' || GetTop(&Ope) != '#')
    {
        if(c>= '0' && c<= '9')
        {
            Push(&Num, Stack_Num, c);
            c = command_char[i];
						i++;
        }

        else
        {
            switch(Judge(GetTop(&Ope), c))
            {
            case '<':
                Push(&Ope, Stack_Ope, c);
                c = command_char[i];
								i++;
                break;
            case '=':
                Pop(&Ope);
                c = command_char[i];
								i++;
                break;
            case '>':
                chOpe = Pop(&Ope);
                a = Pop(&Num);
                b = Pop(&Num);
						    d = Operate(b,chOpe,a);
                Push(&Num, Stack_Num, d);
                break;
            }
        }
    }

		return GetTop(&Num);
}

//*****************************************************************************

//*****************************************************************************
//
// The UART interrupt handler.
//
//*****************************************************************************
void
UARTIntHandler(void)
{
    uint32_t ui32Status, i;
		int32_t j;
	  char result, ch[5];

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
		
		// Flush
		while(ROM_UARTCharsAvail(UART0_BASE))
    {
				ROM_UARTCharGetNonBlocking(UART0_BASE);
		}
		
		if (0 == strcmp(command_char, "READY#"))
		{
			JumpToBootLoader();
			
		}
		else
		{
			result = DoCalculate();
			result -= 48;
			ch[0] = 0;
			j = 0;
			while (result)
			{
				ch[j] = result%10;
				result = result/10;
				j++;
			}
			j--;
			while (j>=0)
			{
				ROM_UARTCharPutNonBlocking(UART0_BASE, (char)(ch[j]+48));
				j--;
			}
		}

}


//*****************************************************************************
//
// The main function.
// A demo calculator.
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
    }
}
