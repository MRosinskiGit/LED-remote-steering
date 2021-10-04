/*
 * Sterowanie_szafa.c
 *
 * Created: 03.10.2021 11:03:44
 * Author : Laptop-Maciej
 */ 
#define F_CPU 8000000UL
#define TRANSISTOR_PWM_PIN PB1                                           //transistor
#define RADIO_INTERRUPT INT0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/common.h>

//#include <avr/delay.h>

                                             //radio input

uint8_t FLAG_State_Request_Change_Remote = 0;
uint8_t FLAG_State_Request_Change_Timer = 0;
uint8_t FLAG_Actual_State = 1;
uint8_t FLAG_Change_In_Progress = 0;
uint8_t FLAG_Change_Prevent_Debounce = 0;
uint8_t ticks = 0;
uint64_t miliseconds = 0;
uint8_t duty = 255;

void turn_on_int0();
void turn_on_timer1();
void turn_off_timer1();
void set_transistor_gate_as_PWM_output_full();
void set_transistor_gate_as_PWM_output_min();
void change_state(int, int);

ISR(INT0_vect) {
	FLAG_State_Request_Change_Remote = 1;
	FLAG_Change_Prevent_Debounce = 1;
	//PORTB ^= (1<<TRANSISTOR_PWM_PIN);
}

ISR(TIMER1_COMPA_vect) {
	miliseconds++;

}

int main(void)
{
	sei();
	DDRB &= ~(1 << RADIO_INTERRUPT);
	DDRB |= (1<<TRANSISTOR_PWM_PIN);
	turn_on_int0(); 

	set_transistor_gate_as_PWM_output_full();
	//PORTB |= (1<<TRANSISTOR_PWM_PIN);
	//turn_on_timer1();
    while (1) 
    {
	
		if (FLAG_State_Request_Change_Remote==1 && FLAG_Actual_State == 0 && FLAG_Change_In_Progress == 0)
		{
			FLAG_Change_In_Progress = 1;
			change_state(1,5);
		}
		
		if (FLAG_State_Request_Change_Remote==1 && FLAG_Actual_State == 1 && FLAG_Change_In_Progress == 0)
		{
			FLAG_Change_In_Progress = 1;
			change_state(0,5);
		}
		
		if (FLAG_Actual_State == 0 && miliseconds == (3600*1000*4))
		{
			FLAG_Change_In_Progress = 1;
			change_state(1,3600);
		}

		
    }
}


void turn_on_int0() {
	MCUCR |= (1<<ISC01)|(1<<ISC00);
	GIMSK |= (1<<INT0);
	return;
}

void set_transistor_gate_as_PWM_output_full() {
		OCR0B = 0;
		DDRB |= (1<<TRANSISTOR_PWM_PIN);
		TCCR0A |= (1<<COM0B1)|(1<<COM0B0)|(1<<WGM01)|(1<<WGM00);
		TCCR0B |= (1<<CS01);

		return;
}
void set_transistor_gate_as_PWM_output_min(){
		OCR0B = 255;
		DDRB |= (1<<TRANSISTOR_PWM_PIN);
		TCCR0B |= (1<<CS01);
		TCCR0A |= (1<<COM0B1)|(1<<COM0B0)|(1<<WGM01)|(1<<WGM00);
		return;	
}


void turn_on_timer1() {
	TIMSK |= (1<<OCIE1A);
	OCR1A = 60;
	TCCR1 |= (1<<CTC1);
	TCCR1 |= (1<<CS11)|(1<<CS10);
	miliseconds = 0;
}

void turn_off_timer1() {
	TCCR1 &= 0b11110000;
}

void change_state(int target_state, int timeout_in_seconds){
	uint64_t period = timeout_in_seconds * 1000 / 255;
	turn_on_timer1();
	if (target_state == 0)
	{
		//set_transistor_gate_as_PWM_output_full();
		while (duty < 255)
		{
			if ((miliseconds % period) == 0 && miliseconds != 0)
			{	
				miliseconds = 0;
				duty = duty + 1;
				OCR0B = duty;
			}
		}
		//PORTB |= (1<<TRANSISTOR_PWM_PIN);

		FLAG_Actual_State = 0;
		miliseconds = 0;
	}
	
	if (target_state == 1)
	{
		//set_transistor_gate_as_PWM_output_min();
		while (duty > 0)
		{
			if ((miliseconds % period) == 0 && miliseconds != 0)
			{
				miliseconds = 0;
				duty = duty - 1;
				OCR0B = duty;
			}
		}
		//PORTB &= !(1<<TRANSISTOR_PWM_PIN);

		turn_off_timer1();
		FLAG_Actual_State = 1;

	}
	FLAG_Change_In_Progress = 0;
	FLAG_State_Request_Change_Remote = 0;
	FLAG_State_Request_Change_Timer = 0;
	return;
}