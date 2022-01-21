#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <LCD.h>
#include <LiquidCrystal.h> 
#include <EEPROM.h>
#include <avr/wdt.h>
#include <MsTimer2.h>

//ESTADOS
#define FABRICA 0
#define RESETADO 1
#define DESCIDA_SALVADA 2
#define SUBINDO 3
#define ELEVADOR_EM_CIMA 4
#define DESCENDO 5
#define ELEVADOR_EM_BAIXO 6
#define SOBRECARGA 7


//Variaveis de estado
typedef struct {

    int motor = 0; 
    int auxMaxCorda = 0;
    volatile char onOff;
    bool salvandoFlag = 0; 

    volatile int posicaoAtual = 0;
    volatile int posicaoFinal = 0;
    volatile char state;

} Varal;

// FUNCOES
void fimDescida();

void fimSubida();

void BrownOutDetect();

void holeCounter();

void calcula();

void salva_descida();

void salva_subida();

void func_descida();

void func_subida();

void func_reset();

#endif  /* FUNCTIONS_H_ */