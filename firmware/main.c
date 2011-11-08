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

/* Configure the ADC to 12b, differential w/ gain, signed mode with a 2.5VREF. */
void initADC(void){
    ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc | 1 << ADC_CONMODE_bp | 0 << ADC_IMPMODE_bp | ADC_CURRLIMIT_NO_gc | ADC_FREERUN_bm;
	ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm; // use 1V internal ref, enable internal reference
//    ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm; // use 2.5VREF at AREFA
    ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc; // ADC CLK = 500KHz
    ADCA.EVCTRL = ADC_SWEEP_012_gc ;
    ADCA.CH0.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_64X_gc;
    ADCA.CH1.CTRL = ADC_CH_INPUTMODE_DIFFWGAIN_gc | ADC_CH_GAIN_1X_gc;
	ADCA.CH3.CTRL = ADC_CH_INPUTMODE_INTERNAL_gc;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc | ADC_CH_MUXNEG_PIN6_gc; // thermocouple 
    ADCA.CH1.MUXCTRL = 0b100 |  ADC_CH_MUXPOS_PIN1_gc; // amplifier
	ADCA.CH2.MUXCTRL = ADC_CH_MUXINT_TEMP_gc; // internal temperature
    ADCA.CTRLA = ADC_ENABLE_bm;
}

/* configure SPI on USARTE0 for communicating with the MAX31855 thermocouple sensor */
void initSPI(void){
	PORTC.DIRSET = 1 << 0 | 1 << 1; //CS, SCK as outputs
	USARTC0.CTRLC = USART_CMODE_MSPI_gc | 1 << 1; // SPI master, MSB first, sample on falling clock (UCPHA=1)
	USARTC0.BAUDCTRLA = 15;  // 1MHz SPI clock. XMEGA AU manual 23.15.6 & 23.3.1
	USARTC0.BAUDCTRLB =  0;
	USARTC0.CTRLB = USART_TXEN_bm | USART_RXEN_bm; // enable TX
	PORTC.OUTCLR = 1 << 1; // SCK low
	PORTC.OUTSET = 1 << 0; // CS high
}

/* return a 16b value containing the 14b temperature reading */
void readSPI(void){

	PORTC.OUTCLR = 1 << 0; // CS low
	_delay_us(10);
	USARTC0.DATA = 0x00;
	while(!(USARTC0.STATUS & USART_DREIF_bm)); // wait until we can write another byte
	USARTC0.STATUS = USART_TXCIF_bm; // clear TX complete flag
	ep0_buf_in[0] = USARTC0.DATA;

	USARTC0.DATA = 0x00;
	while(!(USARTC0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTC0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[1] = USARTC0.DATA;
	
	USARTC0.DATA = 0x00;
	while(!(USARTC0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTC0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[2] = USARTC0.DATA;
	
	USARTC0.DATA = 0x00;
	while(!(USARTC0.STATUS & USART_DREIF_bm)); // wait for TX complete flag
	USARTC0.STATUS = USART_TXCIF_bm;
	ep0_buf_in[3] = USARTC0.DATA;
	
	_delay_us(10);	
	PORTC.OUTSET = 1 << 0; // CS high
}

void configHardware(void){
	initSPI();
	initADC();
	USB_ConfigureClock();
	USB_Init();
}

/** Event handler for the library USB Control Request reception event. */
bool EVENT_USB_Device_ControlRequest(USB_Request_Header_t* req){
	if ((req->bmRequestType & CONTROL_REQTYPE_TYPE) == REQTYPE_VENDOR){
		switch(req->bRequest){

			case 0xA0: 
				readSPI(); // MAX31855
				USB_ep0_send(4);
				break;

			case 0xB0:
				ep0_buf_in[0] = ADCA.CH0.RESH; // thermocouple
				ep0_buf_in[1] = ADCA.CH0.RESL;
				ep0_buf_in[2] = ADCA.CH2.RESL; // local temp
				ep0_buf_in[3] = ADCA.CH2.RESH;
				USB_ep0_send(4);
				break;

			case 0xC0:
				ep0_buf_in[0] = ADCA.CH1.RESH; // amplifier
				ep0_buf_in[1] = ADCA.CH1.RESL; 
				USB_ep0_send(4);
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

