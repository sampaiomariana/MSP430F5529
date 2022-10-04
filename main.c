
// Visto 3 - Mariana Sampaio
// LCD
// P3.0 = SDA
// P3.1 = SCL
// SW alterna entre VRx e VRy

#include <msp430.h>
#include <stdint.h>

#define TRUE    1
#define FALSE   0

// Bits para controle do LCD
#define BIT_RS   BIT0
#define BIT_RW   BIT1
#define BIT_E    BIT2
#define BIT_BL   BIT3

#define BR10K  105 // (SMCLK) 1.048.576/105 ~= 10kHz
#define BR100K  11 // (SMCLK) 1.048.576/11  ~= 100kHz
#define BR100  10 // (SMCLK) 1.048.576/1000  ~= 100Hz

// Bits para controle do Joystick
#define FECHADA 0       // SW fechada
#define ABERTA  1       // SW aberta
#define DBC     1000    // Debounce

#define servo_pwm 16 //0.5x10^-3x32768
#define second 32768
#define max 655 //20ms -> 20x10^-3x32768

// Funções
void lcd_inic(void);
void lcd_aux(char dado);
void i2c_write(char dado);
char i2c_read(void);
char i2c_test(char adr);
void USCI_B0_config(void);
void leds_config(void);
void atraso(long x);

void lcd_char(char dado);
void lcd_string(char *string);
void mode_1_string(char *line, char a, float tensao, int media);

//Funções para o Joystick
int sw_mon(void);
void ADC_config(void);
void TA0_config(void);
void GPIO_config(void);
void debounce(int valor);

//Servo
void prog_servo();

volatile int media_x, media_y, flag = 0;

int main(void){
    // Joystick
    char modo = 1;
    volatile float tensao_x, tensao_y;
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    GPIO_config();
    TA0_config();
    ADC_config();
 //   prog_servo();
    __enable_interrupt();

    // LCD
    char adr;       // Endereço do LCD

    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    leds_config();              // Leds
    USCI_B0_config();           // Configurar USCI_B0

    if      (i2c_test(0x27) == TRUE) adr=0x27; // Verificar o endereço do LCD
    else if (i2c_test(0x3F) == TRUE) adr=0x3F;
    else{
        P1OUT |= BIT0;  // Indicativo de problema
        while(TRUE);    // Travar execução
    }
    P4OUT |= BIT7;      // Verde aceso sinaliza que não foi encontrado nenhum erro
    UCB0I2CSA = adr;    // Seta o endereço a ser usado
    i2c_write(0);       // Todos os pinos em 0
    lcd_inic();

    char first_line[16], second_line[16];

    while(TRUE){
        switch(modo){
            case 1:
                tensao_x = (media_x * 3.3) / 4095;
                tensao_y = (media_y * 3.3) / 4095;

                mode_1_string(first_line, '0', tensao_x, media_x);
                mode_1_string(second_line, '1', tensao_y, media_y);
                break;
        }

        if (flag == 1) {
            lcd_string(first_line);
            lcd_string(second_line);
            P1OUT ^= BIT0;      // Inverte L1
            flag = 0;
        }

        if (sw_mon() == TRUE) {
            modo++;         // Incrementa o modo
            if (modo == 1)  // Limita o modo até 1
                modo = 1;
        }
    }
    return 0;

}
// Joystick
//#pragma vector = 54
#pragma vector = ADC12_VECTOR
__interrupt void adc_int(void){
    volatile unsigned int *pt;
    unsigned int i, soma_x = 0, soma_y = 0;
    pt = &ADC12MEM0;

    for (i = 0; i < 8; i++) {
        soma_x += pt[i];
        soma_y += pt[i + 8];
    }

    media_x = soma_x >> 3;
    media_y = soma_y >> 3;

    flag = 1;
}

// Monitorar SW (P6.3), retorna TRUE se foi acionada
int sw_mon(void){
    static int psw = ABERTA;    // Guardar passado de Sw
    if ((P6IN&BIT3) == 0){      // Qual estado atual de Sw?
        if (psw == ABERTA){     // Qual o passado de Sw?
            debounce(DBC);
            psw = FECHADA;
            return TRUE;
        }
    }
    else{
        if (psw == FECHADA){    // Qual o passado de Sw?
            debounce(DBC);
            psw = ABERTA;
            return FALSE;
        }
    }
    return FALSE;
}

void ADC_config(void){
    volatile unsigned char *pt;
    unsigned char i;
    ADC12CTL0 &= ~ADC12ENC;         // Desabilitar para configurar
    ADC12CTL0 = ADC12ON;            // Ligar ADC
    ADC12CTL1 = ADC12CONSEQ_3 |     // Modo sequência de canais repetido
                ADC12SHS_1 |        // Selecionar TA0.1
                ADC12CSTARTADD_0 |  // Resultado a partir de ADC12MEM0
                ADC12SSEL_3;        // ADC12CLK = SMCLK
    pt = &ADC12MCTL0;
    for (i = 0; i < 8; i++)
        pt[i] = ADC12SREF_0 | ADC12INCH_0;  // ADC12MCTL0 até ADC12MCTL7
    for (i = 8; i < 16; i++)
        pt[i] = ADC12SREF_0 | ADC12INCH_1;  // ADC12MCTL8 até ADC12MCTL15
    pt[15] |= ADC12EOS;     // EOS em ADC12MCTL15
    P6SEL |= BIT2|BIT1;     // Desligar digital de P6.2,1
    ADC12CTL0 |= ADC12ENC;  // Habilitar ADC12
    ADC12IE |= ADC12IE15;   // Hab interrupção MEM2
}

void TA0_config(void){
    TA0CTL = TASSEL_1 | MC_1;
    TA0CCTL1 = OUTMOD_6;    // Out = modo 6
    TA0CCR0 = 32768/32;     // 32 Hz (4 Hz por canal) = 1024
    TA0CCR1 = TA0CCR0/2;    // Carga 50%

    // P2.5 -> TA2.2 -> SERVO */

    //TA2CCTL2 |= OUTMOD_7;   //Servo com disparo automático

}

void GPIO_config(void){
    P6DIR &= ~BIT3;                 // P6.3 = SW
    P6REN |= BIT3;
    P6OUT |= BIT3;
}

// Debounce
void debounce(int valor){
    volatile int x;                 // Volatile evita optimizador
    for (x = 0; x < valor; x++);    // Gasta tempo
}

// Funções para o LCD

// Incializar LCD modo 4 bits
void lcd_inic(void){

    // Preparar I2C para operar
    UCB0CTL1 |= UCTR    |   // Mestre TX
                UCTXSTT;    // Gerar START
    while ( (UCB0IFG & UCTXIFG) == 0);          // Esperar TXIFG=1
    UCB0TXBUF = 0;                              // Sa�da PCF = 0;
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);   // Esperar STT=0
    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG)    // NACK?
                while(1);

    //Inicialização
    lcd_aux(0);     // RS=RW=0, BL=1
    atraso(20000);
    lcd_aux(3);     // 3
    atraso(10000);
    lcd_aux(3);     // 3
    atraso(10000);
    lcd_aux(3);     // 3
    atraso(10000);
    lcd_aux(2);     // 2

    // Entrou em modo 4 bits
    lcd_aux(2);     lcd_aux(8);     // 0x28
    lcd_aux(0);     lcd_aux(8);     // 0x08
    lcd_aux(0);     lcd_aux(1);     // 0x01
    lcd_aux(0);     lcd_aux(6);     // 0x06
    lcd_aux(0);     lcd_aux(0xF);   // 0x0F

    while ( (UCB0IFG & UCTXIFG) == 0)   ;          // Esperar TXIFG=1
    UCB0CTL1 |= UCTXSTP;                           // Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   // Esperar STOP
    atraso(50);
}

// ***Inicialização***
void lcd_aux(char dado){
    while ( (UCB0IFG & UCTXIFG) == 0);              // Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL;          // PCF7:4 = dado;
    atraso(50);
    while ( (UCB0IFG & UCTXIFG) == 0);              // Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL | BIT_E;  // E=1
    atraso(50);
    while ( (UCB0IFG & UCTXIFG) == 0);              // Esperar TXIFG=1
    UCB0TXBUF = ((dado<<4)&0XF0) | BIT_BL;          // E=0;
}

// Gerar START e STOP para inserir o  PCF em estado conhecido
void pcf_stt_stp(void){
    int x=0;
    //UCB0I2CSA = PCF_ADR;      // Endereço Escravo

    while (x<5){
        UCB0CTL1 |= UCTR    |   // Mestre TX
                    UCTXSTT;    // Gerar START
        while ( (UCB0IFG & UCTXIFG) == 0);  // Esperar TXIFG=1
        UCB0CTL1 |= UCTXSTP;                // Gerar STOP
        atraso(200);
        if ( (UCB0CTL1 & UCTXSTP) == 0)   break;    // Esperar STOP
        x++;
    }
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP);       // I2C Travado (Desligar / Ligar)
}

// Escrever dado na porta
void i2c_write(char dado){
    UCB0CTL1 |= UCTR    |   // Mestre TX
                UCTXSTT;    // Gerar START
    while ( (UCB0IFG & UCTXIFG) == 0)   ;          // Esperar TXIFG=1
    UCB0TXBUF = dado;                              // Escrever dado
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT)   ;   // Esperar STT=0
    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG){      // NACK?
        P1OUT |= BIT0;      // NACK=Sinalizar problema
        while(1);           // Travar execução
    }
    UCB0CTL1 |= UCTXSTP;                           // Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP)   ;   // Esperar STOP
}

// Ler a porta do PCF
char i2c_read(void){
    char dado;
    UCB0CTL1 &= ~UCTR;      // Mestre RX
    UCB0CTL1 |= UCTXSTT;    // Gerar START
    while ( (UCB0CTL1 & UCTXSTT) == UCTXSTT);
    if ( (UCB0IFG & UCNACKIFG) == UCNACKIFG){ // NACK?
        P1OUT |= BIT0;      // NACK=Sinalizar problema
        while(1);           // Travar execu��o
    }
    UCB0CTL1 |= UCTXSTP;                // Gerar STOP + NACK
    while ( (UCB0IFG & UCRXIFG) == 0);  // Esperar RX
    dado=UCB0RXBUF;
    while( (UCB0CTL1&UCTXSTP) == UCTXSTP);
    return dado;
}

// Testar o endereço adr para escrita
char i2c_test(char adr){
    UCB0I2CSA = adr;                    // Endereço a ser testado
    UCB0CTL1 |= UCTR;                   // Mestre TX --> escravo RX
    UCB0CTL1 |= UCTXSTT;                // Gerar STASRT
    while ( (UCB0IFG & UCTXIFG) == 0);  // TXIFG --> já iniciou o START
    UCB0CTL1 |= UCTXSTP;                           // Gerar STOP
    while ( (UCB0CTL1 & UCTXSTP) == UCTXSTP);      // Esperar STOP
    if ((UCB0IFG & UCNACKIFG) == 0) return TRUE;   // Chegou ACK
    else                            return FALSE;  // Chegou NACK
}

// Configurar USCI_B0 como mestre
// P3.0 = SDA e P3.1 = SCL
void USCI_B0_config(void){
    UCB0CTL1 = UCSWRST;     // Ressetar USCI_B1
    UCB0CTL0 = UCMST    |   // Modo Mestre
               UCMODE_3 |   // I2C
               UCSYNC;      // Síncrono
   //UCB0BRW = BR10K;      // 10 kbps
    UCB0BRW = BR100K;       // 100 kbps
  //UCB0BRW = BR100;       // 1 kbps

    UCB0CTL1 = UCSSEL_3;    // SMCLK e UCSWRST=0
    P3SEL |= BIT1 | BIT0;   // Funções alternativas
    P3REN |= BIT1 | BIT0;
    P3OUT |= BIT1 | BIT0;
}

// Configurar I/O
void leds_config(void){
    P1DIR |= BIT0; P1OUT &= ~BIT0;  // Led Vermelho
    P4DIR |= BIT7; P4OUT &= ~BIT7;  // Led Verde
}

// Atraso
void atraso(long x){
    volatile long cont=0;
    while(cont++ < x) ;
}

void lcd_char(char dado){
    char data_i2c, msb, lsb;
    data_i2c = BIT_BL | BIT_RS;     // Blacklight e RS

    msb = dado & 0xF0;
    data_i2c |= msb;
    i2c_write(data_i2c);
    data_i2c |= BIT_E;
    i2c_write(data_i2c);
    data_i2c &= ~BIT_E;
    i2c_write(data_i2c);

    data_i2c = BIT_BL | BIT_RS;     // Blacklight e RS
    lsb = (dado << 4) & 0xF0;
    data_i2c |= lsb;
    i2c_write(data_i2c);
    data_i2c |= BIT_E;
    i2c_write(data_i2c);
    data_i2c &= ~BIT_E;
    i2c_write(data_i2c);
   }

void lcd_string(char *string){
    int i;
    for (i = 0; i < 16; i++)    // Insere cada um dos elementos do vetor
        lcd_char(string[i]);
    for (i = 0; i < 24; i++)    // Insere lixo no LCD para alcançar a segunda linha
        lcd_char('X');
}

void mode_1_string(char *line, char a, float tensao, int media){
    line[0] = 'A';
    line[1] = a;
    line[2] = ':';
    line[3] = ' ';
    line[4] = (char)(((int)(tensao * 1000) / 1000) + 48);
    line[5] = ',';
    line[6] = (char)((((int)(tensao * 1000) % 1000) / 100) + 48);
    line[7] = (char)((((int)(tensao * 1000) % 100) / 10) + 48);
    line[8] = (char)(((int)(tensao * 1000) % 10) + 48);
    line[9] = 'V';
    line[10] = ' ';
    line[11] = ' ';

    char media_aux[4];
    int quociente = media, resto, i;
    for (i = 0; i < 4; i++) {
        resto = quociente % 16;
        if (resto < 10)
            media_aux[i] = resto + 48;
        else
            media_aux[i] = resto + 55;
        quociente /= 16;
    }

    line[12] = media_aux[3];
    line[13] = media_aux[2];
    line[14] = media_aux[1];
    line[15] = media_aux[0];
}

void prog_servo(){

    TA2CCR0 = max;
    TA2CCR2 = servo_pwm;
}
