// PWM - Pulse Width Modulation
// This is your first lab in CECS 347 on LaunchPad
// Lab 01 of CECS 347 is to teach the students about PWM
// This lab is based on HelloLaunchPad, DC motor, & SysTick Keil Labs
// Uses HelloLaunchPad as the base lab, uses DC motor for the PWM
// 			and SysTick for those whom need a reminder of how to use SysTick
// If there is no Switch interrupts(SW1 & SW2)
//      the LED toggles High & Low based on the changing Duty Cycle
//			if SW1 is pressed: the high part of the Duty Cycle increases
//				Increases the brightness of the LED
//      if SW2 is pressed: the low part of the Duty Cycle increases
//				Decreases the brightness of the LED

// 0.Documentation Section 
// MainLab1.c
// Runs on LM4F120 or TM4C123
// CECS347Lab1, Input from PF4 & PF0, output to PF1 (LED RED) using PWM
// Author(s): Sean Robinson
// Date: August 29, 2018

// LaunchPad built-in hardware
// SW1 left switch is negative logic PF4 on the Launchpad
// SW2 right switch is negative logic PF0 on the Launchpad
// red LED connected to PF1 on the Launchpad

// Pre-processor Directives Section
// Constant declarations to access port registers using 
// symbolic names instead of addresses
#include <stdint.h>
#include <tm4c123gh6pm.h>
// Defining Port F
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF			0x00000020 // port F Clock Gating Control
// Defining Port F Interrupt
#define GPIO_PORTF_IS_R         (*((volatile unsigned long *)0x40025404))
#define GPIO_PORTF_IBE_R        (*((volatile unsigned long *)0x40025408))
#define GPIO_PORTF_IEV_R        (*((volatile unsigned long *)0x4002540C))
#define GPIO_PORTF_IM_R         (*((volatile unsigned long *)0x40025410))
#define GPIO_PORTF_RIS_R        (*((volatile unsigned long *)0x40025414))
#define GPIO_PORTF_ICR_R        (*((volatile unsigned long *)0x4002541C))
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
// Defining SysTick
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))
// Defining LED
#define RED											0x02
// Defining PWM
#define PWM_2_GENA_ACTCMPAD_ONE 0x000000C0  // Set the output signal to 1
#define PWM_2_GENA_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0
#define PWM_2_GENB_ACTCMPBD_ONE 0x00000C00  // Set the output signal to 1
#define PWM_2_GENB_ACTLOAD_ZERO 0x00000008  // Set the output signal to 0

#define SYSCTL_RCC_USEPWMDIV    0x00100000  // Enable PWM Clock Divisor
#define SYSCTL_RCC_PWMDIV_M     0x000E0000  // PWM Unit Clock Divisor
#define SYSCTL_RCC_PWMDIV_2     0x00000000  // /2


// Function Protoytypes
void PortF_Init(void);
void SysTick_Init(void);
void PWM1A_Init(uint16_t period, uint16_t duty);
void PWM1A_Duty(uint16_t duty);
void EnableInterrupts(void);
void DisableInterrupts(void);
void WaitForInterrupt(void);

// Declarations Section
	// Global Variables
unsigned long H; // Duty Cycle High
unsigned long L; // Duty Cycle Low

// MAIN: Mandatory for a C Program to be executable
int main(void){
	//DisableInterrupts();
	PortF_Init();							// initialize Port F
	PWM1A_Init(16000,8000);
  //SysTick_Init();           // initialize SysTick timer
	GPIO_PORTF_DATA_R = RED;	// initial Color RED High
	//EnableInterrupts();
  while(1){
		WaitForInterrupt();
  }
}


// Subroutine to initialize port F pins for input and output
// PF4 and PF0 are input SW1 and SW2 respectively
// PF1 are outputs to the LED
// Inputs: None
// Outputs: None
// Notes: These five pins are connected to hardware on the LaunchPad
void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) F clock
  delay = SYSCTL_RCGC2_R;           // delay   
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;   // 2) unlock PortF PF0  
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0       
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  //GPIO_PORTF_PCTL_R &= ~0xFFFFFFFF; // 4) GPIO clear bit PCTL 
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 input, PF3,PF2,PF1 output   
  GPIO_PORTF_AFSEL_R = 0x02;        // 6) no alternate function
  GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0       
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0        

  GPIO_PORTF_IS_R &= ~0x11;     		// (d) PF4,PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    		//     PF4,PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    		//     PF4,PF0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      		// (e) clear flags 4,0
  GPIO_PORTF_IM_R |= 0x11;      		// (f) arm interrupt on PF4,PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00400000; // (g) priority 2
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

// L range: 1600, 3200, 4800, 6400, 8000, 9600, 11200, 12800,14400
// power:   10%    20%   30%   40%   50%   60%   70%   80%   90%
void GPIOPortF_Handler(void){   // called on touch of either SW1 or SW2
/*  if(GPIO_PORTF_RIS_R & 0x01){  // SW2 touch
    GPIO_PORTF_ICR_R = 0x01;    // acknowledge flag0
    if(L > 1600){
			L = L - 1600;             // slow down
		}
  }
  if(GPIO_PORTF_RIS_R & 0x10){  // SW1 touch
    GPIO_PORTF_ICR_R = 0x10;    // acknowledge flag4
    if(L < 14400){ 
			L = L + 1600;             // speed up
		}
  }
	// constant period of 1ms, variable duty cycle
  H = 16000 - L;
*/
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

// Subroutine to initialize SysTick
// also sets up a 50% Duty Cycle


/*void SysTick_Init(void){
	H = 8000;									 											// Duty Cycle High 50%
	L = 8000;		
  NVIC_ST_CTRL_R = 0;                   					// disable SysTick during setup
  NVIC_ST_RELOAD_R = L - 1;  											// maximum reload value
  NVIC_ST_CURRENT_R = 0;                					// any write to current clears it
	// enable SysTick with core clock
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x400000000); // priority 2
	NVIC_ST_CTRL_R = 0x07;
}
*/


// Subroutine that is executed by the SysTick
// handles the toggling of the LED
// handles the counting of the Duty Cycles (H & L)


/*void SysTick_Handler(void){
	if(GPIO_PORTF_DATA_R&0x02){   // toggle PF1
    GPIO_PORTF_DATA_R &= ~0x02; // make PF1 low
    NVIC_ST_RELOAD_R = L-1;     // reload value for low phase
  } else{
    GPIO_PORTF_DATA_R |= 0x02;  // make PF1 high
    NVIC_ST_RELOAD_R = H-1;     // reload value for high phase
  }
}
*/


// period is 16-bit number of PWM clock cycles in one period (3<=period)
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
// PWM clock rate = processor clock rate/SYSCTL_RCC_PWMDIV
//                = BusClock/2 
//                = 16 MHz/2 = 8 MHz (in this example)
// Output on PF1/M1PWM5
void PWM1A_Init(uint16_t period, uint16_t duty){
  SYSCTL_RCGCPWM_R |= 0x10;             // 1) activate Module 1
  SYSCTL_RCGCGPIO_R |= 0x00000020;      // 2) activate port F
  while((SYSCTL_PRGPIO_R&0x02) == 0){};
  GPIO_PORTF_AFSEL_R |= 0x02;           // enable alt funct on PF1
  GPIO_PORTF_PCTL_R &= ~0x000000F0;     // configure PF1 as PWM5
  GPIO_PORTF_PCTL_R |=  0x00000050;
  GPIO_PORTF_AMSEL_R &= ~0x02;          // disable analog functionality on PF1
  GPIO_PORTF_DEN_R |= 0x02;             // enable digital I/O on PF1
  SYSCTL_RCC_R =        0x00100000 |    // 3) use PWM divider
      (SYSCTL_RCC_R & (~0x000E0000));   //    configure for /2 divider
  PWM1_2_CTL_R |= 0;                     // 4) re-loading down-counting mode
  PWM1_2_GENA_R = 0xC8;                 // low on LOAD, high on CMPA down
  // PB6 goes low on LOAD
  // PB6 goes high on CMPA down
	PWM1_2_LOAD_R = period - 1;      			// 5) cycles needed to count down to 0
  PWM1_2_CMPA_R = duty - 1;             // 6) count value when output rises
  PWM1_2_CTL_R  |= 0x00000001;           // 7) start PWM
  PWM1_ENABLE_R |= 0x20;          // enable PF1/M1PWM5
}
// change duty cycle of PB6
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
void PWM1A_Duty(uint16_t duty){
  PWM1_2_CMPA_R = duty - 1;             // 6) count value when output rises
}
// Color    LED(s) PortF
// dark     ---    0
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
// yellow   RG-    0x0A
// sky blue -GB    0x0C
// white    RGB    0x0E
// pink     R-B    0x06










