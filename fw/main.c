#include <xc.h>
#define	_XTAL_FREQ	(24000000*1)

// CONFIG1
#pragma config FOSC = HS        // Oscillator Selection (HS Oscillator, High-speed crystal/resonator connected between OSC1 and OSC2 pins)
#pragma config WDTE = OFF        // Watchdog Timer Enable 
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = ON       // PLL Enable (4x PLL enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

#define	BAUD_RATE	57600UL
#define	_delay_us __delay_us
#define	_delay_ms __delay_ms
#define uint8_t unsigned char 
#define uint16_t unsigned int

unsigned char rx_state_machine (unsigned char state, unsigned char rx_char);
void isp_read_pgm (unsigned int * data, unsigned char n);
void isp_write_pgm (unsigned int * data, unsigned char n);
void isp_send_config (unsigned int data);
void isp_mass_erase (void);
void isp_inc_pointer (void);
void isp_reset_pointer (void);
unsigned int isp_read_16 (void);
unsigned int isp_read_14s (void);
void isp_send (unsigned int data, unsigned char n);
unsigned char enter_progmode (void);
unsigned char exit_progmode (void);
void usart_tx_b(uint8_t data);
void usart_tx_s(uint8_t * data);
uint8_t usart_rx_rdy(void);
uint8_t usart_rx_b(void);
void usart_tx_hexa_8 (uint8_t value);
void usart_tx_hexa_8b (uint8_t value);
void usart_tx_hexa_16 (uint16_t value);


#define ISP_PORT	LATA
#define ISP_DDR   TRISA
#define ISP_PIN   PORTA
#define ISP_PORT_M	LATB
#define ISP_DDR_M   TRISB
#define ISP_MCLR  6
#define ISP_DAT   0
#define ISP_CLK   1
#define ISP_LVM   3

#define  ISP_MCLR_1 ISP_PORT_M |= (1<<ISP_MCLR);
#define  ISP_MCLR_0 ISP_PORT_M &= ~(1<<ISP_MCLR);
#define  ISP_MCLR_D_0 ISP_DDR_M &= ~(1<<ISP_MCLR);
#define  ISP_MCLR_D_I ISP_DDR_M |= (1<<ISP_MCLR);

#define  ISP_DAT_1 ISP_PORT |= (1<<ISP_DAT);
#define  ISP_DAT_0 ISP_PORT &= ~(1<<ISP_DAT);
#define  ISP_DAT_V (ISP_PIN&(1<<ISP_DAT))
#define  ISP_DAT_D_0 ISP_DDR &= ~(1<<ISP_DAT);
#define  ISP_DAT_D_I ISP_DDR |= (1<<ISP_DAT);

#define  ISP_CLK_1 ISP_PORT |= (1<<ISP_CLK);
#define  ISP_CLK_0 ISP_PORT &= ~(1<<ISP_CLK);
#define  ISP_CLK_D_0 ISP_DDR &= ~(1<<ISP_CLK);
#define  ISP_CLK_D_I ISP_DDR |= (1<<ISP_CLK);

#define  ISP_LVM_1 ISP_PORT |= (1<<ISP_LVM);
#define  ISP_LVM_0 ISP_PORT &= ~(1<<ISP_LVM);
#define  ISP_LVM_D_0 ISP_DDR &= ~(1<<ISP_LVM);
#define  ISP_LVM_D_I ISP_DDR |= (1<<ISP_LVM);


#define  ISP_CLK_DELAY  1

void isp_send (unsigned int data, unsigned char n);
unsigned int isp_read_16 (void);
unsigned char enter_progmode (void);
unsigned char exit_progmode (void);
void isp_read_pgm (unsigned int * data, unsigned char n);
void isp_write_pgm (unsigned int * data, unsigned char n);
void isp_mass_erase (void);
void isp_reset_pointer (void);

void usart_tx_b(uint8_t data);
uint8_t usart_rx_rdy(void);
uint8_t usart_rx_b(void);
void usart_tx_s(uint8_t * data);

#define _BAUD	57600	// Baud rate (9600 is default)
#define _UBRR	(F_CPU/16)/_BAUD - 1	// Used for UBRRL and UBRRH 


unsigned int dat;
unsigned char rx,i,main_state,bytes_to_receive,rx_state;
unsigned char rx_message[160],rx_message_ptr;
unsigned int flash_buffer[64];
int main (void)
{
ANSELA = 0x00;
ANSELB = 0x00;
TXSTA = 0x24;
RCSTA = 0x90;
BRG16 = 1;
TRISBbits.TRISB5 = 0;
LATBbits.LATB5 = 1;
SPBRGL = (_XTAL_FREQ/BAUD_RATE/4) - 1;	
	
  ISP_CLK_D_0
  ISP_DAT_D_0
  ISP_DAT_0
  ISP_CLK_0
  ISP_MCLR_D_0
  ISP_LVM_D_0
  ISP_LVM_1	  
  ISP_MCLR_0
  _delay_ms(500);
  ISP_MCLR_1
  rx_state = 0;

  while (1)
    {
    if (usart_rx_rdy())
      {
      rx = usart_rx_b();
      rx_state = rx_state_machine (rx_state,rx);
      if (rx_state==3)
        {
        if (rx_message[0]==0x01)
          {
          enter_progmode();
          usart_tx_b (0x81);
          rx_state = 0;
          }
        if (rx_message[0]==0x02)
          {
          exit_progmode();
          usart_tx_b (0x82);
          rx_state = 0;
          }
        if (rx_message[0]==0x03)
          {
          isp_reset_pointer();
          usart_tx_b (0x83);
          rx_state = 0;
          }
        if (rx_message[0]==0x04)
          {
          isp_send_config(0);
          usart_tx_b (0x84);
          rx_state = 0;
          }
        if (rx_message[0]==0x05)
          {
            for (i=0;i<rx_message[2];i++)
            {
            isp_inc_pointer();
            }
          usart_tx_b (0x85);
          rx_state = 0;
          }
        if (rx_message[0]==0x06)
          {
          usart_tx_b (0x86);
          isp_read_pgm(flash_buffer,rx_message[2]);
          for (i=0;i<rx_message[2];i++)
            {
            usart_tx_b (flash_buffer[i]&0xFF);
            usart_tx_b (flash_buffer[i]>>8);
            }
          rx_state = 0;
          }
        if (rx_message[0]==0x07)
          {
          isp_mass_erase();
          usart_tx_b (0x87);
          rx_state = 0;
          }
        if (rx_message[0]==0x08)
          {
          for (i=0;i<rx_message[2]/2;i++)
            {
            flash_buffer[i] = (((unsigned int)(rx_message[(2*i)+1+3]))<<8) + (((unsigned int)(rx_message[(2*i)+0+3]))<<0);
            }
          isp_write_pgm(flash_buffer,rx_message[2]/2);
          usart_tx_b (0x88);
          rx_state = 0;
          }
        }
      }      
    }
}



unsigned char rx_state_machine (unsigned char state, unsigned char rx_char)
{
if (state==0)
  {
    rx_message_ptr = 0;
    rx_message[rx_message_ptr++] = rx_char;
    return 1;
  }
if (state==1)
  {
    bytes_to_receive = rx_char;
    rx_message[rx_message_ptr++] = rx_char;
    if (bytes_to_receive==0) return 3;
    return 2;
  }
if (state==2)
  {
    rx_message[rx_message_ptr++] = rx_char;
    bytes_to_receive--;
    if (bytes_to_receive==0) return 3;
  }
return state;  
}


void isp_read_pgm (unsigned int * data, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)  
  {
  isp_send(0x04,6);
  data[i] = isp_read_14s();
  isp_send(0x06,6);
  }
}




void isp_write_pgm (unsigned int * data, unsigned char n)
{
unsigned char i;
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)  
  {
  isp_send(0x02,6);
  isp_send(data[i]<<1,16);  
  if (i!=(n-1))
    isp_send(0x06,6);
  }
isp_send(0x08,6);
_delay_ms(5);
isp_send(0x06,6);
}

void isp_send_config (unsigned int data)
{
isp_send(0x00,6);
isp_send(data,16);
}

void isp_mass_erase (void)
{
//_delay_ms(10);
//_delay_us(3*ISP_CLK_DELAY);
//isp_send(0x11,6);
isp_send_config(0);
isp_send(0x09,6);
_delay_ms(10);
//isp_send(0x0B,6);
//_delay_ms(10);
}



void isp_reset_pointer (void)
{
//_delay_us(3*ISP_CLK_DELAY);
isp_send(0x16,6);
}

void isp_inc_pointer (void)
{
//_delay_us(3*ISP_CLK_DELAY);
isp_send(0x06,6);
}


unsigned int isp_read_16 (void)
{
unsigned char i;
unsigned int out;
out = 0;
ISP_DAT_D_I
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<16;i++)
  {
  ISP_CLK_1
  _delay_us(ISP_CLK_DELAY);
  ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  out = out >> 1;
  if (ISP_DAT_V)
    out = out | 0x8000;
  }
 return out;
}

unsigned int isp_read_14s (void)
{
unsigned char i;
unsigned int out;
out = isp_read_16();
out = out &0x7FFE;
out = out >> 1;
return out;
}



void isp_send (unsigned int data, unsigned char n)
{
unsigned char i;
ISP_DAT_D_0
//_delay_us(3*ISP_CLK_DELAY);
for (i=0;i<n;i++)
  {
  ISP_CLK_1
  if (data&0x01)
    {
    ISP_DAT_1
    }
  else
    {
    ISP_DAT_0
    }
  _delay_us(ISP_CLK_DELAY);
   ISP_CLK_0
  _delay_us(ISP_CLK_DELAY);
  data = data >> 1;
  }
}

unsigned char enter_progmode (void)
{
ISP_MCLR_0
_delay_us(300);
/*
isp_send(0b10110010,8);
isp_send(0b11000010,8);
isp_send(0b00010010,8);
isp_send(0b00001010,8);
*/
/*
isp_send(0b01001101,8);
isp_send(0b01000011,8);
isp_send(0b01001000,8);
isp_send(0b01010000,8);
*/
/*
isp_send(0b00001010,8);
isp_send(0b00010010,8);
isp_send(0b11000010,8);
isp_send(0b10110010,8);
*/

isp_send(0b01010000,8);
isp_send(0b01001000,8);
isp_send(0b01000011,8);
isp_send(0b01001101,8);

isp_send(0,1);

}

unsigned char exit_progmode (void)
{
ISP_MCLR_1
}



void usart_tx_b(uint8_t data)
{
while (TRMT==0);
TXREG = data;
} 

void usart_tx_s(uint8_t * data)
{
while (*data!=0) 
  usart_tx_b(*data++);
} 


uint8_t usart_rx_rdy(void)
{
if (RCIF)
	return 1;
else
	return 0;
}

uint8_t usart_rx_b(void)
{
return RCREG;
} 


void usart_tx_hexa_8 (uint8_t value)
{
uint8_t temp;
temp = value;
usart_tx_b('0');
usart_tx_b('x');
usart_tx_hexa_8b(value);
usart_tx_b(' ');
}

void usart_tx_hexa_8b (uint8_t value)
{
uint8_t temp;
temp = value;
temp = ((temp>>4)&0x0F);
if (temp<10) temp = temp + '0';
else temp = temp + 'A'- 10;
usart_tx_b(temp);
temp = value;
temp = ((temp>>0)&0x0F);
if (temp<10) temp = temp + '0';
else temp = temp + 'A' - 10;
usart_tx_b(temp);
}


void usart_tx_hexa_16 (uint16_t value)
{
  usart_tx_b('0');
usart_tx_b('x');
  usart_tx_hexa_8b((value>>8)&0xFF);
  usart_tx_hexa_8b(value&0xFF);
  usart_tx_b(' ');
}