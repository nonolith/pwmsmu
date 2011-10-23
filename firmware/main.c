// (C) 2011 Ian Daniher (Nonolith Labs) <ian@nonolithlabs.com>

#define F_CPU 32000000UL
#include <avr/io.h>
#include <avr/delay.h>

#include "Descriptors.h"

#include "usb.h"

/* Function Prototypes: */
void configHardware(void);
void initSPI(void);
void readSPI(void);
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req);

int main(void){
	configHardware();
	sei();	

	while (1){
			USB_Task(); // Lower-priority USB polling, like control requests
	}
}

/* configure SPI on USARTE0 for communicating with the MAX31855 thermocouple sensor */
void initSPI(void){
	PORTE.DIRSET = 1 << 0 | 1 << 1; //CS, SCK as outputs
	USARTE0.CTRLC = USART_CMODE_MSPI_gc | 1 << 1; // SPI master, MSB first, sample on falling clock (UCPHA=1)
	USARTE0.BAUDCTRLA = 15;  // 1MHz SPI clock. XMEGA AU manual 23.15.6 & 23.3.1
	USARTE0.BAUDCTRLB =  0;
	USARTE0.CTRLB = USART_TXEN_bm | USART_RXEN_bm; // enable TX
	PORTE.OUTCLR = 1 << 1; // SCK low
	PORTE.OUTSET = 1 << 0; // CS high
}

/* return a 16b value containing the 14b temperature reading */
void readSPI(void){

	PORTE.OUTCLR = 1 << 0; // CS low
	_delay_us(10);
	USARTE0.DATA = 0x00;
	while(!(USARTE0.STATUS & USART_DREIF_bm)); // wait until we can write another byte
	USARTE0.STATUS = USART_TXCIF_bm; // clear TX complete flag
	ep0_buf_in[0] = USARTE0.DATA;

	USARTE0.DATA = 0x00;
	while(!(USARTE0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTE0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[1] = USARTE0.DATA;
	
	USARTE0.DATA = 0x00;
	while(!(USARTE0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTE0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[2] = USARTE0.DATA;
	
	USARTE0.DATA = 0x00;
	while(!(USARTE0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTE0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[3] = USARTE0.DATA;
	_delay_us(50);	
	PORTE.OUTSET = 1 << 0; // CS high
}

void configHardware(void){
	initSPI();
	USB_ConfigureClock();
	USB_Init();
	PORTB.DIRSET = 1 << 2;
}

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch(req->bRequest){
			case 0xA0: // read MAX31855
				readSPI();
				USB_ep0_send(4);
				break;
			case 0x73: // toggle LED
				PORTB.OUTTGL = 1 << 2;
				USB_ep0_send(0);
				break;
			case 0xBB: // bootload
				USB_ep0_send(0);
				USB_ep0_wait_for_complete();
				_delay_us(10000);
				USB_Detach();
				_delay_us(100000);
				void (*enter_bootloader)(void) = (void *) 0x47fc /*0x8ff8/2*/;
				enter_bootloader();
				break;
		}
		return true;
	}
	return false;
}

