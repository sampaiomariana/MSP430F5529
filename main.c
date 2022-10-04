#include <msp430.h>
#include <intrinsics.h>

void config_pins();
void ta0_config();
void trigger();

volatile unsigned int n0 = 0, n1 = 0, contagem = 0, distancia = 0;
float tempo = 0;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    ta0_config();               //Configurar Timer
    config_pins();              //Configurar o gpio

    while(1) {

        trigger();

        // Verifica os leds
        if (distancia < 10) {
            P1OUT |= BIT0;
            P4OUT |= BIT7;
        }
        else if (distancia < 30) {
            P1OUT |= BIT0;
            P4OUT &= ~BIT7;
        }
        else if (distancia < 50) {
            P1OUT &= ~BIT0;
            P4OUT |= BIT7;
        }
        else {
            P1OUT &= ~BIT0;
            P4OUT &= ~BIT7;
        }

    }

}

void trigger(){
    P1OUT |= BIT5;
     __delay_cycles(10);
    P1OUT &= ~BIT5;

}

void ta0_config(void) {
    /*CONFIG TIMER
     * P1.5 -> TA0.4 -> TRIGGER
     * P2.0 -> TA1.1 -> ECHO */
     TA1CCTL1 = CM_3    |     // Captura ambos os flancos
                 CCIS_0 |     // Seleciona entrada de captura CCI0A (P2.0)
                 SCS    |     // Sincroniza entrada de captura
                 CAP    |     // Seleciona modo captura
                 CCIE;        // Habilita interrupção por captura

      TA1CTL = TASSEL_1 |     // Seleciona ACLK
               ID__4    |     // Seleciona divisor CLK/4 = 8192 Hz
               MC_2     |     // Seleciona modo contínuo
               TACLR;         // Zera contador TA0R


    __enable_interrupt();


}

#pragma vector = TIMER1_A1_VECTOR //48
__interrupt void isr0_ta0 (void) {
    TA1IV;                              // Zera a flag CCIFG

    n1 = TA1CCR1;                      // Pega o valor atual do contador

    contagem = n1 - n0;                  // Calcula as contagens
    tempo = (float)contagem / 8192;     // Encontra quanto tempo demorou para o sinal voltar em segundos
    distancia = tempo * 34300;          // Considerando a velocidade do som em 34300 cm/s é feito a conta para encontrar a distância percorrida pelo sinal
    distancia /= 2;                     // Encontra a distância que se quer medir do objeto

    n0 = n1;

    TA1CCR0 = 0;
}

void config_pins(){

    P1DIR |= BIT0;      // Led Vermelho = P1.0 = Saída
    P1OUT &= ~BIT0;     // Led Vermelho Apagado

    P4DIR |= BIT7;      // Led Verde = P.4.7 = Saída
    P4OUT &= ~BIT7;     // Led Vermelho Apagado

    //Trigger
    P1DIR |= BIT5;
    P1OUT &= ~BIT5;

    //Echo
    P2DIR &= ~BIT0;
    P2REN |= BIT0;   //habilita o pull up
    P2OUT |= BIT0;
    P2SEL |= BIT0;

}
