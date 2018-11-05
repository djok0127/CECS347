// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Test the switch initialization functions by setting the LED
// color according to the status of the switches.
// Daniel and Jonathan Valvano
// October 24, 2018

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2013
   Section 4.2    Program 4.1

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// negative logic switches connected to PF0 and PF4 on the Launchpad
// red LED connected to PF1 on the Launchpad
// blue LED connected to PF2 on the Launchpad
// green LED connected to PF3 on the Launchpad
// NOTE: The NMI (non-maskable interrupt) is on PF0.  That means that
// the Alternate Function Select, Pull-Up Resistor, Pull-Down Resistor,
// and Digital Enable are all locked for PF0 until a value of 0x4C4F434B
// is written to the Port F GPIO Lock Register.  After Port F is
// unlocked, bit 0 of the Port F GPIO Commit Register must be set to
// allow access to PF0's control registers.  On the LM4F120, the other
// bits of the Port F GPIO Commit Register are hard-wired to 1, meaning
// that the rest of Port F can always be freely re-configured at any
// time.  Requiring this procedure makes it unlikely to accidentally
// re-configure the JTAG pins as GPIO, which can lock the debugger out
// of the processor and make it permanently unable to be debugged or
// re-programmed.

#include <stdint.h>
#include <math.h>
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "Nokia5110.h"


#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PF0       (*((volatile uint32_t *)0x40025004))
#define PF4       (*((volatile uint32_t *)0x40025040))
#define SWITCHES  (*((volatile uint32_t *)0x40025044))
#define SW1       0x01                      // on the left side of the Launchpad board
#define SW2       0x10                      // on the right side of the Launchpad board
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control
#define SYSCTL_RCGC2_GPIOB			0x00000002	// port B Clock Gating Control
#define SYSCTL_TCGC2_FPIOA			0x00000001	// port A Clock Gating Control
#define RED       0x02
#define BLUE      0x04
#define GREEN     0x08

// global variable and arrays
volatile unsigned long ADCvalue;
volatile unsigned long ADC_V;
uint16_t calc_distance;
uint16_t adcTable[] = {3333, 2333, 1633, 1332, 1108, 1012, 909, 809, 738, 692, 650, 629, 586};
uint16_t distTable[] = {10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70};
uint16_t table_dist;


void ADC_InitSeq3Ch9(void);
uint32_t ADC0_InSeq3(void);
void Systick_Inter_Init(void);
void Delay(unsigned long ulCount);
void SysTick_Handler(void);
float Convert(float sample);

int main(void){
	PLL_Init();														// initialize PLL for 50MHz
	ADC_InitSeq3Ch9();										// initialize ADC 0 Seq 3 Channel 9
	Nokia5110_Init();											// initialize the nokia display
  Nokia5110_Clear();										// clear the nokia screen
  Nokia5110_OutChar(127);               // print UT sign
	Systick_Inter_Init();									// initialize system tick for 20Hz, 50ms
	while(1){
		Nokia5110_SetCursor(1, 0); 					// one leading spaces, first row
		Nokia5110_OutString("ADC : ");
		Nokia5110_OutUDec(ADCvalue);
		Nokia5110_OutString("V : ");
		Nokia5110_OutUDec(Convert(ADCvalue));		
		Nokia5110_SetCursor(1, 2);          // one leading spaces, second row
		Nokia5110_OutString("Calc: ");
		Nokia5110_OutUDec(calc_distance);
		Nokia5110_SetCursor(1, 3);          // one leading spaces, third row
		Nokia5110_OutString("Table:");
		Nokia5110_OutUDec(table_dist);                    
    Delay(833333);                     
	}
	
}
float Convert(float sample){
	
  return sample/4095.0* 3.3;  // replace this line with real code
}
void ADC_InitSeq3Ch9(void){
	volatile unsigned long delay;
	SYSCTL_RCGCGPIO_R |= 0x10;        // 1) activate Port E clock
	while((SYSCTL_PRGPIO_R & 0x10) == 0);
	GPIO_PORTE_DIR_R &= ~0x10;				// 2) make PE4 input
	GPIO_PORTE_AFSEL_R |= 0x10;				// 3) enable alternate function of PE4
	GPIO_PORTE_DEN_R &= ~0x10;				// 4) disable PE4 digital I/O
	GPIO_PORTE_AMSEL_R |= 0x10;				// 5) enable analog function on PE4
	SYSCTL_RCGCADC_R |= 0x01;					// 6) activate ADC0
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	ADC0_PC_R = 0x01;									// 7) configure for 125k sample rate
	ADC0_SSPRI_R = 0x0123; 						// 8) Seq 3 is highest priority
	ADC0_ACTSS_R &= ~0x008;							// 9) disable sample seq 3;
	ADC0_EMUX_R &= ~0xF000;						// 10) seq3 is software trigger
	ADC0_SSMUX3_R = (ADC0_SSMUX3_R & 0xFFFFFFF0) // 11) clear SS3 field
										+9; 
	ADC0_SSCTL3_R = 0x0006;						// 12) no TS0 D0, yes IE0 END0
	ADC0_IM_R &= ~0x0008;							// 13) disable SS3 interrupt
	ADC0_ACTSS_R |= 0x0008;						// 14) enable sample sequencer 3
}

uint32_t ADC0_InSeq3(void){
	uint32_t result;									// initializing a variable name
	ADC0_PSSI_R = 0x0008;							// set the flag for ADC0 seq 3
	while((ADC0_RIS_R & 0x08) == 0);	// waiting for the flag to be high
	result = ADC0_SSFIFO3_R & 0xFFF;	// gives the value from the sample
	ADC0_ISC_R = 0x0008;							// reset the flag for ADC0 seq 3
	return result;										// return result of the sequance
}

void Systick_Inter_Init(void){
	NVIC_ST_CTRL_R = 0;								// disable systick for setup
	NVIC_ST_RELOAD_R = 2500000;						// set the load for 20Hz,50ms
	NVIC_SYS_PRI3_R = 1 <<29;					// set the priority to level to 3
	NVIC_ST_CTRL_R = 7;								// enable count down, interrupt, systick
}

//  function delays 3*ulCount cycles
void Delay(unsigned long ulCount){
  do{
    ulCount--;
	}while(ulCount);
}

void SysTick_Handler(void){
	int8_t i;
	uint8_t indexL = 0;
	uint8_t indexH = 12;
	ADCvalue = ADC0_InSeq3();											// receiving the ADC value from the sensor
	calc_distance = pow(ADCvalue,-1.207) * 155876;// equation to find the distance
	for(i=0; i < 13; i++){												// finding the closest lowest value of ADC value
		if(ADCvalue < adcTable[i])
			indexL = i;
		if(ADCvalue > adcTable[i])									// breaks from for loop once found lowest value
			break;
	}
	for(i=12; i >= 0; i--){												// finding the closest highest value of ADC value
		if(ADCvalue > adcTable[i])
			indexH = i;
		if(ADCvalue < adcTable[i])									// breaks from for loop once found highest value
			break;
	}
	// finding the distance from the the distance table array
	if((calc_distance - distTable[indexL]) < (distTable[indexH] - calc_distance))
		table_dist = distTable[indexL];
	else if((calc_distance - distTable[indexL]) > (distTable[indexH] - calc_distance))
		table_dist = distTable[indexH];
}

