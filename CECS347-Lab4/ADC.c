// ADC.c
// Runs on LM4F120/TM4C123
// Provide functions that initialize ADC0 SS3 to be triggered by
// software and trigger a conversion, wait for it to finish,
// and return the result. 
// Daniel Valvano
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
#include "stdint.h"
#include "ADC.h"
#include "tm4c123gh6pm.h"

// This initialization function sets up the ADC 
// Max sample rate: <=125,000 samples/second
// SS3 triggering event: software trigger
// SS3 1st sample source:  channel 1
// SS3 interrupts: enabled but not promoted to controller
int delay;
void ADC0_Init(void){ 
	SYSCTL_RCGCGPIO_R |= 0x10;		// 1)activate clock for Port E
	while((SYSCTL_PRGPIO_R&0x10) == 0){};
	GPIO_PORTE_DIR_R &= ~0x10;      // 2) make PE4 input
  GPIO_PORTE_AFSEL_R |= 0x10;     // 3) enable alternate fun on PE4
  GPIO_PORTE_DEN_R &= ~0x10;      // 4) disable digital I/O on PE4
  GPIO_PORTE_AMSEL_R |= 0x10;     // 5) enable analog fun on PE4
  SYSCTL_RCGCADC_R |= 0x01;       // 6) activate ADC0
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;       // extra time to stabilize
  delay = SYSCTL_RCGCADC_R;
  ADC0_PC_R = 0x01;               // 7) configure for 125K
  ADC0_SSPRI_R = 0x0123;          // 8) Seq 3 is highest priority
  ADC0_ACTSS_R &= ~0x0008;        // 9) disable sample sequencer 3
  ADC0_EMUX_R &= ~0xF000;         // 10) seq3 is software trigger
  ADC0_SSMUX3_R = (ADC0_SSMUX3_R&0xFFFFFFF0)+9;  // 11) Ain9 (PE4)
  ADC0_SSCTL3_R = 0x0006;         // 12) no TS0 D0, yes IE0 END0
  ADC0_IM_R &= ~0x0008;           // 13) disable SS3 interrupts
  ADC0_ACTSS_R |= 0x0008;         // 14) enable sample sequencer 3
}

//------------ADC0_In------------
// Busy-wait Analog to digital conversion
// Input: none
// Output: 12-bit result of ADC conversion


uint32_t ADC0_In(void){
	uint32_t data;  
	ADC0_PSSI_R = 0x0008;
	while((ADC0_RIS_R & 0x08) == 0 ){};
	data = ADC0_SSFIFO3_R & 0xFFF;
	ADC0_ISC_R = 0x0008;
	return data;
}

// consider using this block
void ADC_InitSeq2Ch1_0_9(void){
	volatile unsigned long delay;
	SYSCTL_RCGCGPIO_R |= 0x10;        // 1) activate Port E clock
	while((SYSCTL_PRGPIO_R & 0x10) == 0);
	GPIO_PORTE_DIR_R &= ~0x1C;				// 2) make PE2,3,4 input
	GPIO_PORTE_AFSEL_R |= 0x1C;				// 3) enable alternate function of PE2,3
	GPIO_PORTE_DEN_R &= ~0x1C;				// 4) disable PE4 digital I/O
	GPIO_PORTE_AMSEL_R |= 0x1C;				// 5) enable analog function on PE4
	SYSCTL_RCGCADC_R |= 0x01;					// 6) activate ADC0
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	delay = SYSCTL_RCGCADC_R;
	ADC0_PC_R = 0x01;									// 7) configure for 125k sample rate
	ADC0_SSPRI_R = 0x3210; 						// 8) Seq 3 is highest priority
	ADC0_ACTSS_R &= ~0x004;							// 9) disable sample seq 2;
	ADC0_EMUX_R &= ~0x0F00;						// 10) seq3 is software trigger
	ADC0_SSMUX2_R = (ADC0_SSMUX2_R & 0xFFFFFFFF); // 11) clear SS3 field
	ADC0_SSMUX2_R = 0x0901;
	ADC0_SSCTL2_R = 0x0600;						// 12) no TS0 D0, yes IE0 END0
	ADC0_IM_R &= ~0x0004;							// 13) disable SS2 interrupt
	ADC0_ACTSS_R |= 0x0004;						// 14) enable sample sequencer 2
}

void ADC0_InSeq2(unsigned long *ch1value, unsigned long *ch0value, unsigned long *ch9value){
	ADC0_PSSI_R = 0x0004;								// set the flag for ADC0 seq 2
	while((ADC0_RIS_R&0x04) == 0);			// waiting for the flag to be high
	*ch1value = ADC0_SSFIFO2_R & 0xFFF;
	*ch0value = ADC0_SSFIFO2_R & 0xFFF;
	*ch9value = ADC0_SSFIFO2_R & 0xFFF;
	ADC0_ISC_R = 0x0004;								// reset the flag for ADC0 seq 3
}