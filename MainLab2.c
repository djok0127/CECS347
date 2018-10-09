// This is your second program to run on the LaunchPad
// You will run this program with modification as your Lab 2
// If SW1
//			increase the High duty cycle - increasing brightness
// If SW2
//			decreases the High duty cycle - dimming brightness
// The modifications from lab 1 is that of using the on-board
// PWM instead of building one using the SysTick Module.
// This modification will later be used to control motors
// for a remote control vehicle.

// 0.Documentation Section 
// MainLab2.c
// Runs on LM4F120 or TM4C123
// MainLab2, Input from PF4 & PF0, output to PF1 (LED) using on-board PWM
// Author(s): Sean Robinson
// Note: Allan Montalvo helped me to understand this lab02. He helped me understand
//       that depending on the the order you initialize it will affect the outcome.
// Date: August 29, 2018

// LaunchPad built-in hardware
// SW1 left  switch is negative logic PF4 on the Launchpad
// SW2 right switch is negative logic PF0 on the Launchpad
// red LED connected to PF1 on the Launchpad

#include <stdint.h>
#include <tm4c123gh6pm.h>

#define SYSCTL_RCGC2_GPIOF			0x00000020  // Port F Clock Gating Control
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register

uint16_t H;

void PortF_Init(void){ 
	volatile unsigned long delay;
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGC2_GPIOF;	// Port F Clock Set 0x00000020
  delay = SYSCTL_RCGC2_R;           				// Delay   
	GPIO_PORTF_LOCK_R  = GPIO_LOCK_KEY;				// Unlock PortF PF0 0x4C4F434B
  GPIO_PORTF_CR_R    =  0x1F;           		// Allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R =  0x00;        				// Disable analog function
  GPIO_PORTF_DIR_R   =  0x0E;          			// PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R =  0x02;        				// Alternate functionality
	GPIO_PORTF_PCTL_R &= ~0x000000F0;     		// Configure PF1 as PWM5
  GPIO_PORTF_PCTL_R |=  0x00000050;
  GPIO_PORTF_PUR_R   =  0x11;          			// Enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R   =  0x1F;          			// Enable digital pins PF4-PF0        
	
  GPIO_PORTF_IS_R  &= ~0x11;     				  // PF4,PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    				  // PF4,PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    				  // PF4,PF0 falling edge event
  GPIO_PORTF_ICR_R  =  0x11;      			  // Clear flags 4,0
  GPIO_PORTF_IM_R  |=  0x11;      			  // Arm interrupt on PF4,PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)| // Priority: 2
														 0x00400000; 
  NVIC_EN0_R        = 0x40000000;      	  // Enable interrupt 30 in NVIC
}

// PWM Module 1 A Initialize
	// This module 
	// input(s): uint16_t period & uint16_t duty
	// output(s): none
void PWM1A_Init(uint16_t period, uint16_t duty){
	H = 8000;
	SYSCTL_RCGCPWM_R   |= 2;     					// Activate PWM1
  SYSCTL_RCC_R        = 0x00100000 |    // Use PWM divider
      (SYSCTL_RCC_R & (~0x000E0000));   //    configure for /2 divider
  PWM1_2_CTL_R       |= 0;              // Re-loading down-counting mode	
	PWM1_2_GENB_R       = 0x0000080C;    // Low on LOAD, high on CMPA down
  // PF1 goes low on LOAD
  // PF1 goes high on CMPB down
  PWM1_2_LOAD_R       = period - 1;     // Cycles needed to count down to 0
  PWM1_2_CMPB_R       = duty - 1;       // Count value when output rises
  PWM1_2_CTL_R       |= 1;    					// Start PWM1
  PWM1_ENABLE_R      |= 0x20; 			    // Enable PF1/M1PWM5
	}

// PWM Module 1 A
	// This module changes the comparator value of PWM1 i.e. changing the
	// time of the duty cycle being "High" for each period
	// Input(s): uint16_t duty
	// Output(s): none
	//
void PWM1A_Duty(uint16_t duty){
  PWM1_2_CMPB_R = duty - 1;             // Count value when output rises
}
	
// L range: 1600, 3200, 4800, 6400, 8000, 9600, 11200, 12800,14400
// power:   10%    20%   30%   40%   50%   60%   70%   80%   90%
void GPIOPortF_Handler(void){   // called on touch of either SW1 or SW2
		if(GPIO_PORTF_RIS_R & 0x01){
			GPIO_PORTF_ICR_R = 0x01;
			if(H < 14400){
				H = H + 1600;
			}
		}
		if(GPIO_PORTF_RIS_R & 0x10){
			GPIO_PORTF_ICR_R = 0x10;
			if(H > 1600){
				H = H - 1600;
			}
		}
		PWM1A_Duty(H);
}

// MAIN: Mandatory for a C Program to be executable
int main(void){
	PortF_Init();							// Initialize Port F
	PWM1A_Init(16000,8000);		// Initialize PWM1A
  while(1);
}
