// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to periodically initiate a software-
// triggered ADC conversion, convert the sample to a fixed-
// point decimal distance, and store the result in a mailbox.
// The foreground thread takes the result from the mailbox,
// converts the result to a string, and prints it to the
// Nokia5110 LCD.  The display is optional.
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// Slide pot pin 3 connected to +3.3V
// Slide pot pin 2 connected to PE2(Ain1) and PD3
// Slide pot pin 1 connected to ground


#include "ADC.h"
#include "PLL.h"
#include <math.h>
#include <stdint.h>
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "TExaS.h"

void EnableInterrupts(void);  // Enable interrupts
void SysTick_Init(unsigned long);
unsigned char String[10]; // null-terminated ASCII string
unsigned long Distance;   // units 0.001 cm
unsigned long ADCdata;    // 12-bit 0 to 4095 sample
unsigned long Flag;       // 1 means valid Distance, 0 means Distance is empty
void Delay(unsigned long ulCount);

// global variable and arrays
volatile unsigned long ADCvalue;
int calc_distance;
uint16_t adcTable[] = {3333, 2333, 1633, 1332, 1108, 1012, 909, 809, 738, 692, 650, 629, 586};
uint16_t distTable[] = {10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70};
uint16_t table_dist;

//********Convert****************
// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm).  Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.  
// Overflow and dropout should be considered 
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
unsigned long Convert(unsigned long sample){
	
  return 4095 * sample / 3.3;  // replace this line with real code
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
void SysTick_Init(unsigned long period){
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_RELOAD_R = period -1;
	NVIC_ST_CURRENT_R = 0;
	NVIC_SYS_PRI3_R = 0x00000007;
}
// executes every 25 ms, collects a sample, converts and stores in mailbox
void SysTick_Handler(void){
	
	int i;
	uint8_t indexL = 0;
	uint8_t indexH = 12;
	ADCvalue = ADC0_In();											// receiving the ADC value from the sensor
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

//-----------------------UART_ConvertDistance-----------------------
// Converts a 32-bit distance into an ASCII string
// Input: 32-bit number to be converted (resolution 0.001cm)
// Output: store the conversion in global variable String[10]
// Fixed format 1 digit, point, 3 digits, space, units, null termination
// Examples
//    4 to "0.004 cm"  
//   31 to "0.031 cm" 
//  102 to "0.102 cm" 
// 2210 to "2.210 cm"
//10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
void UART_ConvertDistance(unsigned long n){
// as part of Lab 11 you implemented this function
// string for the ASCII	String[10];

}

/*
// main1 is a simple main program allowing you to debug the ADC interface
int main1(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  EnableInterrupts();
  while(1){ 
    ADCdata = ADC0_In();
  }
}
// once the ADC is operational, you can use main2 to debug the convert to distance
int main2(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_NoScope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  Nokia5110_Init();             // initialize Nokia5110 LCD
  EnableInterrupts();
  while(1){ 
    ADCdata = ADC0_In();
    Nokia5110_SetCursor(0, 0);
    Distance = Convert(ADCdata);
    UART_ConvertDistance(Distance); // from Lab 11
    Nokia5110_OutString(String);    // output to Nokia5110 LCD (optional)
  }
}
*/
// once the ADC and convert to distance functions are operational,
// you should use this main to build the final solution with interrupts and mailbox
int main(void){ 
  volatile unsigned long delay;
  //TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope); 
	PLL_Init();
// initialize ADC0, channel 1, sequencer 3
	ADC0_Init();

  EnableInterrupts();
	Nokia5110_Init();											// initialize the nokia display
  Nokia5110_Clear();										// clear the nokia screen
  Nokia5110_OutChar(127);               // print UT sign
	SysTick_Init(2500000);									// initialize system tick for 20Hz, 50ms
	
	while(1){
		
		Nokia5110_SetCursor(1, 0); 					// one leading spaces, first row
		Nokia5110_OutString("ADC : ");
		Nokia5110_OutUDec(ADCvalue);
		Nokia5110_SetCursor(1, 1);          // one leading spaces, second row
		Nokia5110_OutString("Dist: ");
		Nokia5110_OutUDec(calc_distance);
		Nokia5110_SetCursor(1, 2);          // one leading spaces, third row
		Nokia5110_OutString("TD   :");
		Nokia5110_OutUDec(table_dist);                    
    Delay(833333);                     
	}
}

//  function delays 3*ulCount cycles
void Delay(unsigned long ulCount){
  do{
    ulCount--;
	}while(ulCount);
}

