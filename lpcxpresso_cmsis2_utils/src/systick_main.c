// CMSIS headers required for setting up SysTick Timer
#include "LPC17xx.h"

#include <cr_section_macros.h>
#include <NXP/crp.h>

// Variable to store CRP value in. Will be placed automatically
// by the linker when "Enable Code Read Protect" selected.
// See crp.h header for more information
__CRP const unsigned int CRP_WORD = CRP_NO_CRP ;

#include "leds.h"
#include "uart.h"
#include "logger.h"
#include "s0_input.h"

volatile uint32_t msTicks; // counter for 1ms SysTicks
extern volatile unsigned int eint3_count;
extern volatile uint32_t UART2Count, UART0Count, UART1Count, UART3Count;
extern volatile uint8_t UART2Buffer[BUFSIZE], UART0Buffer[BUFSIZE], UART1Buffer[BUFSIZE], UART3Buffer[BUFSIZE];



// ****************
//  SysTick_Handler - just increment SysTick counter
void SysTick_Handler(void) {
  msTicks++;
}

// ****************
// systick_delay - creates a delay of the appropriate number of Systicks (happens every 10 ms)
__INLINE static void systick_delay (uint32_t delayTicks) {
  uint32_t currentTicks;

  currentTicks = msTicks;	// read current tick counter
  // Now loop until required number of ticks passes.
  while ((msTicks - currentTicks) < delayTicks);
}

// ****************
int main(void) {
	
	// Setup SysTick Timer to interrupt at 10 msec intervals
	if (SysTick_Config(SystemCoreClock / 100)) {
	    while (1);  // Capture error
	}

	led_init();	// Setup GPIO for LED2
	led2_on();		// Turn LED2 on
	//led_on(0);
	//led_on(1);

	systick_delay(100);
	led2_off();
	systick_delay(100);
	led2_off();


	UARTInit(0, 115200);	/* baud rate setting */
	UARTInit(1, 9600);	/* baud rate setting, PC */
	UARTInit(2, 9600);	/* baud rate setting, RS485*/
	UARTSendCRLF(0);
	UARTSendCRLF(0);
	UARTSendStringln(0, "UART2 online ...");

	//EINT3_init();





	// Enter an infinite loop, just incrementing a counter and toggling leds every second
	//led2_off();
	//int ledstate;


	//EINT3_enable();
	logger_logStringln("logger online ...");
	uint8_t data, datac, i1, i2;
	uint8_t hex_data[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	while(1) {

		/* process logger */
		if (logger_dataAvailable() && UARTTXReady(0)) {
			uint8_t data = logger_read();
			UARTSendByte(0,data);
		}

		process_leds(msTicks);

		process_s0(msTicks);

		uint32_t triggerValue = s0_triggered(0);
		if (triggerValue) {
			logger_logString("s0_0:");
			logger_logNumberln(triggerValue);
			led_signal(0, 30, msTicks);
		}

		triggerValue = s0_triggered(1);
		if (triggerValue) {
			logger_logString("s0_1:");
			logger_logNumberln(triggerValue);
			led_signal(1, 30, msTicks);
		}


		if ( UART0Count != 0 ) {
			led2_on();
			LPC_UART0->IER = IER_THRE | IER_RLS;				/* Disable RBR */

			int i = 0;
			for(; i < UART0Count; i++) {
				logger_logByte(UART0Buffer[i]);
			}
			UART0Count = 0;
			LPC_UART0->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */
			led2_off();
		}

		/* UART1 is PC */
		if ( UART1Count != 0 ) {
			//led2_on();
			LPC_UART1->IER = IER_THRE | IER_RLS;				/* Disable RBR */

			int i = 0;
			led_on(7);
			led_on(0);
			for(; i < UART1Count; i++) {
				/* forward to RS485 */
				/* convert to 7e1 */
				//data = UART1Buffer[i] & 0b01111111;
				data = UART1Buffer[i];
				datac = data & 0b01111111;
				UARTSendByte(2, data);
				logger_logByte(datac);
			}
			led_off(0);
			led_off(7);
			UART1Count = 0;
			LPC_UART1->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */
		}

		/* UART2 is RS485 */
		if ( UART2Count != 0 ) {
			led_signal(1, 300, msTicks);
			led2_on();
			LPC_UART2->IER = IER_THRE | IER_RLS;				/* Disable RBR */

			int i = 0;
			for(; i < UART2Count; i++) {
				/* forward to PC */
				//data = UART2Buffer[i] & 0b01111111;
				data = UART2Buffer[i];
				datac = data & 0b01111111;
				i1 = (datac & 0x0F);
				i2 = (datac >> 4) & 0x0F;
				logger_logByte(hex_data[i2]);
				logger_logByte(hex_data[i1]);
				logger_logCRLF();
				//logger_logNumberln(datac);
			}
			UART2Count = 0;
			LPC_UART2->IER = IER_THRE | IER_RLS | IER_RBR;		/* Re-enable RBR */
			led2_off();
		}


	}
	return 0 ;
}
