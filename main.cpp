#include "mbed.h"

#define BLINKING_RATE     500ms
#define T_REBOTE          5ms

//declarar pines 
DigitalOut sclk(D2);
DigitalInOut dio(D3);
DigitalIn btn_incrementar(BUTTON1);      // boton unidad
DigitalIn btn_reiniciar(D4);           // boton reiniciar
DigitalIn btn_detener(D5);             // boton detener automatico
DigitalIn btn_automatico(D6);          // boton contar automatico
DigitalIn  dip_suma(D7);               //dip sitwch define si aumenta o disminuye    



const char digitToSegment[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; 
static bool Q0=0,Q1=0,Q2=0;
static int contador = 0;      
static bool conteo_automatico = 0; 

// hilos y funciones 
Thread hilo_rebote;    //hilo para el antirebote del boton de unidad
Thread hilo_automatico;  //para hacer el conteo automatico

void leer_pin(void);
void cond_start(void);
void cond_stop(void);
void send_byte(char data);
void send_data(int number);
void incrementar(void);
void reiniciar(void);
void controlar_automatico(void);


int main()
{
    printf("Arranque del sistema \n\r");
    //arrancar hilos 
    hilo_rebote.start(leer_pin);         
    hilo_automatico.start(controlar_automatico);  

    while (true) {
        
    }
}

void cond_start(void)
{
    sclk = 1;
    dio.output();
    dio = 1;
    wait_us(1);
    dio = 0;
    sclk = 0;
}

void cond_stop(void)
{
    sclk = 0;
    dio.output();
    dio = 0;
    wait_us(1);
    sclk = 1;
    dio = 1;
}

void send_byte(char data)
{
    dio.output();
    for (int i = 0; i < 8; i++)
    {
        sclk = 0;
        dio = (data & 0x01) ? 1 : 0;
        data >>= 1;
        wait_us(1);  
        sclk = 1;
        wait_us(1);
    }

    //esperar el ACK
    dio.input();
    sclk = 0;
    wait_us(1);
    sclk = 1;
    wait_us(1);
}

void send_data(int number)
{
    cond_start();
    send_byte(0x40);  // se envian todos los datos el 0x42 se tiene que enviar uno a uno 
    cond_stop();
    cond_start();
    send_byte(0xc0);  //enciendde y prepara
    
    int digit[4] = {0, 0, 0, 0};  //vector para ubicar los dijitos de manera correctar por la construccion del tm1637
    
    for (int i = 3; i >= 0; i--) {   //con este se invienten las posiciones 
        digit[i] = number % 10;
        number /= 10;
    }
    for (int i = 0; i < 4; i++) {
        send_byte(digitToSegment[digit[i]]);       //se envian
    }
    cond_stop();
    cond_start();
    send_byte(0x88 + 4);  //  el brillo 
    cond_stop();
}

void leer_pin(void)
{
    while (true) {
        
        Q2 = Q1;// Z[n-2]
        Q1 = Q0;   // Z[n-1]
        Q0 = btn_incrementar; // Z[n]

        if (Q0 && !Q1 && !Q2) {
            printf("El conteo es: %u \n\r", contador+1);
            incrementar();  // llama la funcion incrementar que lo que hace es contar
        }

        
        if (btn_reiniciar) {   //si el bototn reiniciar se oprime
            reiniciar();  // llama la funcion que pone el contador en 0 
            ThisThread::sleep_for(200ms);  
        }

        
        if (btn_detener) {
            conteo_automatico = 0;  
            printf("Conteo automatico detenido \n\r");
            ThisThread::sleep_for(200ms); 
        }

        
        if (btn_automatico) {
            conteo_automatico = 1;  
            printf("Conteo automático iniciado \n\r");
            ThisThread::sleep_for(200ms); 
        }

        ThisThread::sleep_for(T_REBOTE);
    }
}

void incrementar(void)
{
    if (dip_suma == 1) {
        contador++;  
        
    } else {
        if (contador < 0) contador = 9999;  
        contador--;  
        
    }
    send_data(contador); 
    
}

void reiniciar(void)
{
    contador = 0;  
    send_data(contador);  
    printf("Contador reiniciado \n\r");
}

void controlar_automatico(void)
{
    while (true) {
        if (conteo_automatico ==1 ) {
            incrementar();  // llama la funcion que cuenta
            ThisThread::sleep_for(1s);  // aca coloco la velocidad a la que quiero que cuente 
        } else {
            //no hace nada 
        }
    }
}