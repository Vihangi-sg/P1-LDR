
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define Push_Button1 (1<<PD2)
#define Push_Button2 (1<<PD3)

void usart_begin(void);
void usart_send(unsigned char i); 
void adc_init1();
void adc_init2();
float adc_read ();

unsigned char overflowCount = 0;
int cycles = 5;
char Label_LDR;

void adc_init1(){
  ADMUX = 1<<MUX0| 1<<REFS0; // Select the corresponding ADC channel and Set reference voltage to AVcc
  DIDR0 |= 1<<ADC1D;
  ADCSRA = 1<<ADEN|1<<ADPS0|1<<ADPS1|1<<ADPS2; // Enable ADC and set prescaler to 128
}

void adc_init2(){
  ADMUX = 1<<MUX1| 1<<REFS0; // Select the corresponding ADC channel and Set reference voltage to AVcc
  DIDR0 |= 1<<ADC2D; 
  ADCSRA = 1<<ADEN|1<<ADPS0|1<<ADPS1|1<<ADPS2; // Enable ADC and set prescaler to 128
}

void usart_begin(void){
  UCSR0B |= (1<<TXEN0); // Enable transmitter
  UCSR0C |= 1<<UCSZ01|1<<UCSZ00;  // Set frame format to 8 data bits, 1 stop bit
  UBRR0 = 103; // Set baud rate to 9600
}

void usart_send(unsigned char i){
  while(!(UCSR0A & (1<<UDRE0))); // Wait until the transmit buffer is empty
    UDR0 = i; 
}

float adc_read(){  
  ADCSRA |= 1<<ADSC;  // Start conversion
  while (!(ADCSRA & (1<<ADIF))); // Wait for conversion to complete
  ADCSRA &= ~(1<<ADIF);
  return ADC;
}

void timer1_init() {
  TCCR1B |= (1 << WGM12); // Set the mode to CTC
  OCR1A = 15625; // Set the compare match register for 1s interval
  TIMSK1 |= (1 << OCIE1A); // Enable timer compare interrupt
  TCCR1B |= (1 << CS12) | (1 << CS10); // Set the prescaler to 1024
}

int main (void){
  //Configures the pins for the push buttons as inputs and Enables the internal pull-up resistors.
  DDRD &= ~Push_Button1;
  PORTD |= Push_Button1;
  DDRD &= ~Push_Button2;
  PORTD |= Push_Button2;

  timer1_init(); //Initializes the Timer1
  EIMSK = 1<<INT0|1<<INT1; //Enables external interrupts INT0 and INT1 for the push buttons.
  EICRA = 1<<ISC01 | 1<<ISC11; //Configures the interrupt to trigger on the falling edge.
  sei(); // Enable global interrupts

  while (1){ // Main loop does nothing, everything is handled by interrupts
  }
}

// Timer1 compare match interrupt service routine
ISR(TIMER1_COMPA_vect){
  overflowCount++;
  if (overflowCount >= cycles){ // After 5 overflows (5 seconds)
    overflowCount = 0; // Reset overflow counter
    usart_begin ();
    adc_init1();
    unsigned short adc_value1 = adc_read(); // Read from ADC channel 1
    adc_init2();
    unsigned short adc_value2 = adc_read(); // Read from ADC channel 2
    float x;
    // Compare and transmit the higher value
    if (adc_value1 > adc_value2) {
      x = adc_value1; 
      Label_LDR = '1';
    } 
    else {
      x = adc_value2;
      Label_LDR = '2';
    }
    // Format the string to send: "LDRx: value"
    usart_send ('L');
    usart_send ('D');
    usart_send ('R');
    usart_send (Label_LDR);
    usart_send (':');
    usart_send (' ');

    float adc_value = (x-8)/1007; // Map the ADC value (0-1023) to a range between 0 and 1
    char str[5] = {0,0,0,0,0};
    dtostrf(adc_value, 5, 3, str);
    for (unsigned char i = 0; i < 5; i++){
      usart_send (str[i]);
    }
    usart_send (' ');
  }
}

ISR(INT0_vect){
  // Push_Button1 pressed, decrease interval
  cycles -= 1;
  if(cycles <= 1){
    cycles = 1;
  }
}

ISR(INT1_vect){
  // Push_Button2 pressed, increase interval
  cycles += 1;
  if (cycles >= 10){
    cycles = 10;
  }
}