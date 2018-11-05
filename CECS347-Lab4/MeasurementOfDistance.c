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
#include "tm4c123gh6pm.h"
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
// Convert ADC to digital number 0-4095
// Input: Analog Sample
// Output: Digital Number from 0 to 4095
unsigned long Convert(unsigned long sample){
	
  return 4095 * sample / 3.3; // converts 0 - 3.3V input into a digital number
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
void SysTick_Init(unsigned long period){
	NVIC_ST_CTRL_R = 0;								// disable systick for setup
	NVIC_ST_RELOAD_R = period;						// set the load for 20Hz,50ms
	NVIC_SYS_PRI3_R = 1 << 29;					// set the priority to level to 3
	NVIC_ST_CTRL_R = 7;								// enable count down, interrupt, systick
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

// TODO: Change with Jordan's Code. There is something wrong with my code
//------------------------------------------------- PWM Motor ------------------------------------------
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define PWM_0_GENA_ACTCMPAD_ONE 0x000000C0  // Set the output signal to 1
#define PWM_0_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
#define PWM_0_GENB_ACTCMPBD_ONE 0x00000C00  // Set the output signal to 1
#define PWM_0_GENB_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0

#define PWM_1_GENA_ACTCMPAD_ONE 0x000000C0  // Set the output signal to 1
#define PWM_1_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
#define PWM_1_GENB_ACTCMPBD_ONE 0x00000C00  // Set the output signal to 1
#define PWM_1_GENB_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0

#define SYSCTL_RCC_USEPWMDIV    0x00100000  // Enable PWM Clock Divisor
#define SYSCTL_RCC_PWMDIV_M     0x000E0000  // PWM Unit Clock Divisor
#define SYSCTL_RCC_PWMDIV_2     0x00000000  // /2

uint16_t H;
int speed, direction;
// DeBounce
unsigned char p_count, r_count, pressed, released; 
// Initialize PortF Handler
#define clear	0x00;
unsigned short count = 0;
unsigned short dir = 'F';

// PWM Module 1 A Initialize
	// This module 
	// input(s): uint16_t period & uint16_t duty
	// output(s): none
void PWM1A_Init(uint16_t period, uint16_t duty){
	H = 8000;
	SYSCTL_RCGCPWM_R   |= 2;     					// Activate PWM1
  SYSCTL_RCC_R        = 0x00100000 |    // Use PWM divider
      (SYSCTL_RCC_R & (~0x000E0000));   //    configure for /2 divider
  PWM0_1_CTL_R       |= 0;              // Re-loading down-counting mode	
	PWM0_1_GENB_R       = 0x0000080C;    // Low on LOAD, high on CMPA down
  // PF1 goes low on LOAD
  // PF1 goes high on CMPB down
  PWM0_1_LOAD_R       = period - 1;     // Cycles needed to count down to 0
  PWM0_1_CMPA_R			  = duty - 1;
	PWM0_1_CMPB_R       = duty - 1;       // Count value when output rises
  PWM0_1_CTL_R       |= 1;    					// Start PWM1
  PWM0_ENABLE_R      |= 0x02; 			    // Enable PB7/M0PWM1
	}

// PWM Module 1 A
	// This module changes the comparator value of PWM1 i.e. changing the
	// time of the duty cycle being "High" for each period
	// Input(s): uint16_t duty
	// Output(s): none
	//
void PWM1A_Duty(uint16_t duty){
	PWM0_1_CMPA_R = duty - 1;
  PWM0_1_CMPB_R = duty - 1;             // Count value when output rises
}

void PWM0A_Init(unsigned int period, unsigned int duty){
  SYSCTL_RCGCPWM_R |= 0x01;             // 1) activate PWM0
  SYSCTL_RCGCGPIO_R |= 0x02;            // 2) activate port B
  while((SYSCTL_PRGPIO_R&0x02) == 0){};
  GPIO_PORTB_AFSEL_R |= 0xC0;           // enable alt funct on PB6
  GPIO_PORTB_PCTL_R &= ~0xFF000000;     // configure PB6 as PWM0
  GPIO_PORTB_PCTL_R |= 0x44000000;
  GPIO_PORTB_AMSEL_R &= ~0xC0;          // disable analog functionality on PB6
  GPIO_PORTB_DEN_R |= 0xC0;             // enable digital I/O on PB6
  SYSCTL_RCC_R = 0x00100000 |           // 3) use PWM divider
      (SYSCTL_RCC_R & (~0x000E0000));   //    configure for /2 divider
  PWM0_0_CTL_R = 0;                     // 4) re-loading down-counting mode
  PWM0_0_GENA_R = 0xC8;                 // low on LOAD, high on CMPA down
  // PB6 goes low on LOAD
  // PB6 goes high on CMPA down
  PWM0_0_LOAD_R = period - 1;           // 5) cycles needed to count down to 0
  PWM0_0_CMPA_R = duty - 1;             // 6) count value when output rises
  PWM0_0_CTL_R |= 0x00000001;           // 7) start PWM0
  PWM0_ENABLE_R |= 0x00000001;          // enable PB6/M0PWM0
}



// TODO: change the duty according to the datat that was read in from the adc
void PWM0A_Duty(unsigned int duty){
  PWM0_0_CMPA_R = duty - 1;             // 6) count value when output rises
}


int main(void){ 
	
  volatile unsigned long delay;
	
  // TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope); 
	PLL_Init();
	// initialize ADC0, channel 1, sequencer 3
	ADC0_Init();
	
	PWM0A_Init(16000,7);
	
	PWM0A_Duty(3);
	PWM1A_Duty(3);

	
  //EnableInterrupts();
	Nokia5110_Init();											// initialize the nokia display
  Nokia5110_Clear();										// clear the nokia screen
  Nokia5110_OutChar(127);               // print UT sign
	SysTick_Init(4000000);									// initialize system tick for 20Hz, 50ms
	
	while(1){
		
		
		// out of range
		// TODO: need an if
		//Nokia5110_SetCursor(1, 0); 					// display out of range on the first row
		//Nokia5110_OutString("out of");
		//Nokia5110_SetCursor(1, 1);          // display out of range on the second row
		//Nokia5110_OutString("range");
		// in range else
		//Nokia5110_SetCursor(1, 0); 					
		//Nokia5110_OutString("r: ");					// display data recived from the left sensor on the first row
		//Nokia5110_SetCursor(1, 1);          
		//Nokia5110_OutString("l: ");					// display data recived from the left sensor second row
		
		//Nokia5110_SetCursor(1, 2);					// set cursor to third row
		//Nokia5110_OutString("PWM: ");				// PWM percentage
		
		// need to get PWM Data from adc
		//Nokia5110_OutUDec(PWM_DATA);
		
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

