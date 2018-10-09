// This is your first program to run on the LaunchPad
// You will run this program without modification as your Lab 2
// If the left switch SW1 is 
//      not pressed the LED toggles blue-red
//      pressed the LED toggles blue-green

// 0.Documentation Section 
// CECS347Lab1.c
// Runs on LM4F120 or TM4C123
// CECS347Lab1, Input from PF4 & PF0, output to PF3,PF2,PF1 (LED) using PWM
// Author(s): Sean Robinson
// Date: August 29, 2018

// LaunchPad built-in hardware
// SW1 left switch is negative logic PF4 on the Launchpad
// SW2 right switch is negative logic PF0 on the Launchpad
// red LED connected to PF1 on the Launchpad
// blue LED connected to PF2 on the Launchpad
// green LED connected to PF3 on the Launchpad

// 1. Pre-processor Directives Section
// Constant declarations to access port registers using 
// symbolic names instead of addresses
//#include "TExaS.h"
// PF Defined
#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_LOCK_KEY           0x4C4F434B  // Unlocks the GPIO_CR register
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_GPIOF			0x00000020 // PF Clock Gating Control
// PF SW Interrupt Defined
#define GPIO_PORTF_IS_R         (*((volatile unsigned long *)0x40025404))
#define GPIO_PORTF_IBE_R        (*((volatile unsigned long *)0x40025408))
#define GPIO_PORTF_IEV_R        (*((volatile unsigned long *)0x4002540C))
#define GPIO_PORTF_IM_R         (*((volatile unsigned long *)0x40025410))
#define GPIO_PORTF_RIS_R        (*((volatile unsigned long *)0x40025414))
#define GPIO_PORTF_ICR_R        (*((volatile unsigned long *)0x4002541C))
// SysTick Defined
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
// SysTick Priority Defined
#define NVIC_EN0_R              (*((volatile unsigned long *)0xE000E100))
#define NVIC_PRI7_R             (*((volatile unsigned long *)0xE000E41C))
#define NVIC_SYS_PRI3_R         (*((volatile unsigned long *)0xE000ED20))

// 2. Declarations Section
//   Global Variables
unsigned long H; // Duty Cycle High
unsigned long L; // Duty Cycle Low

//   Function Prototypes
void DisableInterrupts(void); // disable interrupts
void EnableInterrupts(void);  // enable interrupts
void WaitForInterrupt(void);  // low power mode

// Subroutine to initialize port F pins for input and output
// PF4 and PF0 are input SW1 and SW2 respectively
// PF3,PF2,PF1 are outputs to the LED
// Inputs: None
// Outputs: None
// Notes: These five pins are connected to hardware on the LaunchPad
void PortF_Init(void){ volatile unsigned long delay;
  // PF General Setup
	SYSCTL_RCGC2_R |= 0x00000020; // (a) activate clock for port F
	delay = SYSCTL_RCGC2_R;
  GPIO_PORTF_LOCK_R = 0x4C4F434B; // unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x13;         // allow changes to PF4,1,0
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x13;  //     disable alt funct on PF4,1,0
  GPIO_PORTF_DEN_R |= 0x13;     //     enable digital I/O on PF4,1,0
  GPIO_PORTF_PCTL_R &= ~0x000F000F; //  configure PF4,0 as GPIO
  GPIO_PORTF_AMSEL_R &= ~0x13;  //     disable analog functionality on PF4,1,0
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4,0
  // PF SW Interrupt Setup
	GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4,PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4,PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4,PF0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flags 4,0
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4,PF0
  // PF Interrupt Priority Setup
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00400000; // (g) priority 2
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
	// PF1 HIGH upon Initialization
	GPIO_PORTF_DATA_R &= 0x02;
}

// L range: 1600, 3200, 4800, 6400, 8000, 9600, 11200, 12800,14400
// power:   10%    20%   30%   40%   50%   60%   70%   80%   90%
void GPIOPortF_Handler(void){   // called on touch of either SW1 or SW2
  if(GPIO_PORTF_RIS_R & 0x01){  // SW2 touch
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
}

// Subroutine to initialize SysTick Timer
// On-Board Frequency is 16 MHz
// Set High/Low of the Duty Cycle to 50%
void SysTick_Init(void){
	H = 8000;									 // Duty Cycle High 50%
	L = 8000;									 // Duty Cycle Low 50%
  NVIC_ST_CTRL_R = 0;        // disable SysTick during setup
  NVIC_ST_RELOAD_R = L - 1;  // maximum reload value
  NVIC_ST_CURRENT_R = 0;     // any write to current clears it
  // enable SysTick with core clock
  NVIC_ST_CTRL_R = 0x00000007;
	EnableInterrupts();
}

// SysTick Handler handles the LED
// PF1 (RED) toggles between H & L of the varying 
// Duty Cycle
void SysTick_Handler(void){
	// Toggling LED (PF1)
	if(GPIO_PORTF_DATA_R & 0x02){   // PF1 High
			GPIO_PORTF_DATA_R &= ~0x02; // Turn off PF1
			NVIC_ST_RELOAD_R = L - 1;		// Acknowledge PF1 Low
		} else{												// PF1 Low
			GPIO_PORTF_DATA_R |= 0x02;	// Turn on PF1
			NVIC_ST_RELOAD_R = H - 1;		// Acknowledge PF1 High
		}
	}

// 3. Subroutines Section
// MAIN: Mandatory for a C Program to be executable
int main(void){    
	//TExaS_Init(SW_PIN_PF40,LED_PIN_PF321); // this initializes the TExaS grader lab 2
	DisableInterrupts();
	PortF_Init(); 
	SysTick_Init();
	while(1){
		WaitForInterrupt(); // low power mode
	}
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

