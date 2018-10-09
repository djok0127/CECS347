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
// CECS347Project1.c
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

#include "stdint.h"
#include "tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "PLL.h"

#define SYSCTL_RCGC2_GPIOF			0x00000020  // Port F Clock Gating Control
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register

uint16_t H;
int speed, direction;
// DeBounce
unsigned char p_count, r_count, pressed, released; 
// Initialize PortF Handler
#define clear	0x00;
unsigned short count = 0;
unsigned short dir = 'F';

void Delay(void);

void PortF_Init(void){ 
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;	// Port F Clock Set 0x00000020
  delay = SYSCTL_RCGC2_R;           				// Delay   
	GPIO_PORTF_LOCK_R  = GPIO_LOCK_KEY;				// Unlock PortF PF0 0x4C4F434B
  GPIO_PORTF_CR_R    =  0x1F;           		// Allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R =  0x00;        				// Disable analog function
  GPIO_PORTF_DIR_R   =  0x0E;          			// PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R =  0x00;        				// Alternate functionality
	GPIO_PORTF_PCTL_R  = 0x00000000;     			// Deassert the PWM
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

void PortB_Init(void){ 
	volatile unsigned long delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;	// Port F Clock Set 0x00000020
  delay = SYSCTL_RCGC2_R;           				// Delay   
  GPIO_PORTB_CR_R    =  0xC0;           		// Allow changes to PB6-7       
  GPIO_PORTB_AMSEL_R =  0x00;        				// Disable analog function
  GPIO_PORTB_DIR_R   =  0x0E;          			// PA4,PA0 input, PF3,PF2,PF1 output   
  GPIO_PORTB_AFSEL_R =  0x00;        				// Alternate functionality
	GPIO_PORTB_PCTL_R &= ~0x0000000F;     		// Configure PA6 aned PA7 PWM 0 and PWM 1
  GPIO_PORTB_PCTL_R |=  0x00000004;    
  GPIO_PORTB_DEN_R   =  0x1F;          			// Enable digital pins PF4-PF0        
	
  GPIO_PORTB_IS_R  &= ~0x11;     				  // PF4,PF0 is edge-sensitive
  GPIO_PORTB_IBE_R &= ~0x11;    				  // PF4,PF0 is not both edges
  GPIO_PORTB_IEV_R &= ~0x11;    				  // PF4,PF0 falling edge event
  GPIO_PORTB_ICR_R  =  0x11;      			  // Clear flags 4,0
  GPIO_PORTB_IM_R  |=  0x11;      			  // Arm interrupt on PF4,PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF0FFFFF)| // Priority: 2
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
  PWM1_2_CMPA_R			  = duty - 1;
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
	PWM1_2_CMPA_R = duty - 1;
  PWM1_2_CMPB_R = duty - 1;             // Count value when output rises
}

void PWM0A_Init(unsigned int period, unsigned int duty){
  SYSCTL_RCGCPWM_R |= 0x01;             // 1) activate PWM0
  SYSCTL_RCGCGPIO_R |= 0x02;            // 2) activate port B
  while((SYSCTL_PRGPIO_R&0x02) == 0){};
  GPIO_PORTB_AFSEL_R |= 0x40;           // enable alt funct on PB6
  GPIO_PORTB_PCTL_R &= ~0x0F000000;     // configure PB6 as PWM0
  GPIO_PORTB_PCTL_R |= 0x04000000;
  GPIO_PORTB_AMSEL_R &= ~0x40;          // disable analog functionality on PB6
  GPIO_PORTB_DEN_R |= 0x40;             // enable digital I/O on PB6
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



void PWM0A_Duty(unsigned int duty){
  PWM0_0_CMPA_R = duty - 1;             // 6) count value when output rises
}
	
// L range: 1600, 3200, 4800, 6400, 8000, 9600, 11200, 12800,14400
// power:   10%    20%   30%   40%   50%   60%   70%   80%   90%
void GPIOPortF_Handler(void){   // called on touch of either SW1 or SW2
	// SW1 will be used to increase the speed of the robot
	// starts with no motion, press once for 25% speed, press
	// again for 50%, press again for 75%, press again for 100%, press again and the
	// robot will stop
	// press again and the cycle repeats
	if(GPIO_PORTF_RIS_R & 0x01){
		
			Delay();
			GPIO_PORTF_ICR_R = 0x01;
			speed = speed + 1;
		
			if(speed == 0){
				GPIO_PORTF_DATA_R = 0x02; // RED
				H = 0;
				count = 0;
			} else if(speed == 1){
				GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R;
				H = 4000;  // 25%
				count = 25;
			} else if(speed == 2){
				GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R;
				H = 8000;  // 50%
				count = 50;
			} else if(speed == 3){
				GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R;
				H = 12000; // 75%
				count = 75;
			} else if(speed == 4){
				GPIO_PORTF_DATA_R = GPIO_PORTF_DATA_R;
				H = 16000; // 100%
			} else {
				speed = 0;
				H = 0;
				count = 0;
				GPIO_PORTF_DATA_R = 0x02;
			}
			PWM1A_Duty(H);
		}
	
		// SW2 will be used to control moving direction of the robot
		// starts forward direction mode, press once for backwards
		// press again and the cycle repeats
		if(GPIO_PORTF_RIS_R & 0x10){
			Delay();
			GPIO_PORTF_ICR_R = 0x10;
			direction ^= 1;
			if(direction == 0){
				GPIO_PORTF_DATA_R = 0x04; // moving backwards (BLUE)
				dir = 'B';
			} else if(direction == 1){
				GPIO_PORTF_DATA_R = 0x08; // moving forwards  (GREEN)
				dir = 'F';
			} else {
				GPIO_PORTF_DATA_R = 0x0E; // ERROR (ALL Colors)
				dir = 'E';
			}
		}
		
		Nokia5110_SetCursor(5, 5);          // five leading spaces, bottom row
		Nokia5110_OutChar(dir);
    Nokia5110_OutChar(' ');
    Nokia5110_OutUDec(count);
}

// Subroutine to wait 0.1 sec
// Inputs: None
// Outputs: None
// Notes: ...
void Delay(void){unsigned long volatile time;
  time = 727240*1.25;  // 0.01sec
  while(time){
		time--;
  }
}

// MAIN: Mandatory for a C Program to be executable
int main(void){
	// Initialize Port F
	PortF_Init();
	// DeBounce
	// PF4: SW1
	p_count=0;r_count=0;
  pressed=0;released = 1;
	// Starting Stop
	speed = 0;     	
	// Starting Forward
	direction = 1; 						
// Initialize LCD Display
	PLL_Init();
	Nokia5110_Init();
	Nokia5110_Clear();
	Nokia5110_OutString("************* LCD DISP *************DIR : SPEED:------- ---- ");
  Nokia5110_OutChar(127);               // print UT sign
	Nokia5110_SetCursor(5,5);
	Nokia5110_OutChar('F');
	Nokia5110_OutChar(' ');
	Nokia5110_OutUDec(0);
	// Initialize PWM1A
	PWM1A_Init(16000,0);

  while(1){
		
		// DeBounce
		if(((GPIO_PORTF_DATA_R == 0x01) && released) || ((GPIO_PORTF_DATA_R == 0x10) && released)){ // PF4 or PF0
			p_count++;
			if(p_count>=80){
				pressed = 1; released = 0;
				p_count = 0; Delay();
			}
		} 
		if((GPIO_PORTF_DATA_R == 0x11) && pressed){ // If nothing is pressed
			r_count++;
			if(r_count>=80){
				pressed = 0; released = 1;
				r_count = 0; Delay();
			}
		}
	} // while(1) end
} // main() end

