#include <msp430g2353.h>

unsigned char msbyte;
unsigned char ack;
unsigned int i=0;

void init( void )
{
DCOCTL = 0; // Next 4 lines set digital oscillator to 1MHz
BCSCTL3=0;
BCSCTL1=CALBC1_1MHZ;
DCOCTL =CALDCO_1MHZ;

  
 // set P1 to special function for UART
 P1SEL    = (BIT1 | BIT2);
 P1SEL2   = (BIT1 | BIT2);
 UCA0CTL1 = UCSWRST;     // keeps UART bit in reset while being configured in next 4 lines
 UCA0CTL1 |= UCSSEL_2;   // Set the UART clock to be the SMCLK
 UCA0BR0  = 103;         // Sets Baud rate to 9600 by looking at MSP430 User guide Table 
 UCA0BR1  = 0;           // Higher byte of BR control register
 UCA0MCTL = UCBRS0;      // Sets Modulation stage
 P2OUT &= 0x00;          // set the output register to ground
 

  
}

void i2c_delay(void){
}

void i2c_start(void){
  
  P2DIR &= ~0x01;  // set the P2.0 (SDA) as input for SDA = 1 due to pull up resistor;
  i2c_delay(); 
  P2DIR &= ~0x02; // set the P2.1 (SCL) as input for SCL = 1  due to pull up resistor;
  i2c_delay();
  P2DIR |= 0x01; // Set SDA as output for  SDA = 0 due to P2OUT being set to ground earlier
  i2c_delay();
  P2DIR |= 0x02; // Set P2.1 (SCL)  as output
  i2c_delay(); 
}

void i2c_stop(void){
  P2DIR |= 0x01; // set SDA = 0 
  i2c_delay();
  P2DIR &= ~0x02; // set SCL = 1
  i2c_delay();
  P2DIR &= ~0x01; // set SDA = 1
  i2c_delay();
  
}
unsigned char i2c_write(unsigned char WrittenReg){
  int x;
  unsigned char acknowledge;
  for (x=8; x; x--){
    if(WrittenReg&0x80){ // if the written 8 bit register bitwise anded with 1000 0000 is true then the MSB of SDA is 1
        P2DIR &= ~0x01; // SDA = 1
    }
    else{ // if false MSB of WrittenReg is 0
      P2DIR |= 0x01;  // SDA = 0
    }
    P2DIR &= ~0x02; //  SCL=1
    WrittenReg <<= 1; // shift the desired written reg as logical left 1 for next bit
    P2DIR |= 0x02;  // SCL = 0;
     
  }
  P2DIR &= ~0x03;  // SDA = 1 & SCL = 1
  i2c_delay();
  acknowledge = P2IN&0x01; // Save acknowledge bit in case needed 
  
  P2DIR |= 0x02;  // SCL = 0
  return acknowledge;
}
unsigned char i2c_read(char readacbit){
  int j;
  int ReadRegister = 0;
  P2DIR &= ~0x01;   // Set SDA as input in order to read the line
  for (j=0;j<8;j++){
    
   P2DIR &= ~0x02;
   // this if statement allows for "clock stretching" by the slave
    if(!(P2IN&0x02)){
      P2DIR &= ~0x02;
    }
    
    
   // now SCL = 1 since it is out of the if statement
     
     i2c_delay();
    
    if(P2IN&0x01){        // If the SDA line is high then the slave sent a 1 bit to the Master 
      ReadRegister |= 1;  // OR the ReadRegister variable with 1 to give a value of 1 for the current bit in ReadRegistor
    }
    else{
      ReadRegister |= 0;
    }
    ReadRegister <<=1;  // shift the variable that will store the received byte 
    P2DIR |= 0x02;     // SCL = 0
  }
    if(readacbit){
      P2DIR |= 0x01; // if the master acknowledge is 1 then set the SDA line low for more data to be sent by slave
    }
    else{
      P2DIR &= ~0x01;  // Set SDA high so slave will stop sending data
    }
  P2DIR &= ~0x02;  // Set SCL=1
  i2c_delay();
  P2DIR |= 0x02;  // SCL = 0
  i2c_delay();
  P2DIR &= ~0x01; // SDA = 1
  return ReadRegister;
 }
void accelinit(void){
  //Initialize Acceleromter for 6DOF board for measurement mode by setting the measure bit high in register 0x2D
   i2c_start();
   i2c_write(0xA6);
   i2c_write(0x2D);
   i2c_write(0x08);
   i2c_stop();
}
void readaccel(void){
  i2c_start();
  i2c_write(0xA6);
  i2c_write(0x32);
  i2c_start();
  i2c_write(0xA7);
  msbyte=i2c_read(0);
  i2c_stop();

    
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  init(); 
  UCA0CTL1 &= ~UCSWRST;
  accelinit();
  
  
  while(1)
  {
   readaccel();
   UCA0TXBUF=msbyte;
   while(UCA0STAT&0x01);
   for(i=0;i<200;i++);// delay for a little bit before reading again
  }


}
