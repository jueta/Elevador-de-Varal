
// ELEVADOR DE VARAL

//=============================================================== 


#define MAX_CORDA 120

//BIBLIOTECAS
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include<LCD.h>
#include <LiquidCrystal.h> 
#include <EEPROM.h>
#include <avr/wdt.h>
#include <MsTimer2.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

//ESTADOS
#define FABRICA 0
#define RESETADO 1
#define DESCIDA_SALVADA 2
#define SUBINDO 3
#define ELEVADOR_EM_CIMA 4
#define DESCENDO 5
#define ELEVADOR_EM_BAIXO 6
#define SOBRECARGA 7

//PINOS
const int RPWM_Output = 5; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM) 
const int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM) 
const int BOTAO_SOBE = 3; 
const int BOTAO_LIGA = 0; 
const int BOTAO_DESCE = 4; 
const int BOTAO_RESET = 10; 
const int BOTAO_SALVA_SOBE = 11;
const int BOTAO_SALVA_DESCE = 12;
const int encoder = 2;  // Mudar para pino 2 ou 3
const int voltageSensor = A0;
const int powerOffSensor = A2;


//VARIAVEIS
int motor = 0;   
int auxMaxCorda = 0;
bool salvandoFlag = 0;
volatile unsigned long posicaoAtual = 0;
volatile unsigned long posicaoFinal = 0;
const float VCC  = 5.0; // supply voltage is from 4.5 to 5.5V. Normally 5V.
const float QOV =  0.5 * VCC; // set quiescent Output voltage of 0.5V.
float voltage; // internal variable for voltage  
float current = 0;
float voltage_raw = 0;
unsigned long timeCounter;


//Variaveis de estado
volatile char state;
volatile char onOff;



void setup() { 

    Serial.begin(9600); 

    lcd.begin(16, 2);     
    lcd.clear(); 
    lcd.setCursor(3, 0);
    lcd.setBacklight(LOW);

    onOff = 0;

    posicaoAtual = EEPROM.read(0);
    posicaoFinal = EEPROM.read(1);
    state = EEPROM.read(2);

    pinMode(RPWM_Output, OUTPUT); 
    pinMode(LPWM_Output, OUTPUT); 
    pinMode(BOTAO_SOBE, INPUT); 
    pinMode(BOTAO_LIGA, INPUT); 
    pinMode(BOTAO_DESCE, INPUT); 
    pinMode(BOTAO_RESET, INPUT); 
    pinMode(BOTAO_SALVA_SOBE, INPUT); 
    pinMode(BOTAO_SALVA_DESCE, INPUT); 
    pinMode(voltageSensor,INPUT); 
    pinMode(encoder,INPUT);

    wdt_enable(WDTO_8S);

    attachInterrupt(digitalPinToInterrupt(encoder), holeCounter, RISING);

    MsTimer2::set(15, BrownOutDetect); // 1ms period
    MsTimer2::start();
}


void BrownOutDetect() {    //detecta a queda de tensao no pino A3 e salva tudo

    if(analogRead(powerOffSensor) < 920) {

        pinMode(powerOffSensor, OUTPUT);
        digitalWrite(powerOffSensor, HIGH);
        return;
    }

    pinMode(powerOffSensor, INPUT);

    if(analogRead(powerOffSensor)  > 1000){
        EEPROM.write(0, posicaoAtual); // Salva a posicao que parou
        EEPROM.write(1, posicaoFinal);
        EEPROM.write(2,state);
        lcd.setCursor(10, 1); 
        lcd.print(analogRead("Saved"));
    }
}

void holeCounter() {    // Contador de furos do Encoder   **ISR**
    
    if(salvandoFlag == true){ // Is saving
        
        if (state == SUBINDO) {
            posicaoAtual--;
        }

        if (state == DESCENDO) {
            posicaoAtual++;

            //auxMaxCorda++; 

            // if(auxMaxCorda == MAX_CORDA){
            //     lcd.clear();
            //     lcd.setCursor(0, 0);
            //     lcd.print("Chegou ao Maximo");
            //     lcd.setCursor(0, 1); 
            //     lcd.print("Descida salva");  

            //     analogWrite(LPWM_Output, 0); 
            //     analogWrite(RPWM_Output, 0); 
            //     motor = 0;

            //     state = DESCIDA_SALVADA;
            //     posicaoFinal = posicaoAtual;
            //     EEPROM.write(0, posicaoAtual); // Salva a posicao que parou
            //     EEPROM.write(1, posicaoFinal);
            //     EEPROM.write(2,state);
            // }

        }
    } else {

        if (state == DESCENDO) {
            
            posicaoAtual++;

            if(posicaoAtual == posicaoFinal){
                analogWrite(LPWM_Output, 0); 
                analogWrite(RPWM_Output, 0); 
                motor = 0;
                
                lcd.clear();
                lcd.setCursor(0, 0); 
                lcd.print("Varal embaixo");
                lcd.setCursor(0, 1); 
                lcd.print("Tecle subir"); 

                state = ELEVADOR_EM_BAIXO;
                EEPROM.write(0, posicaoAtual);
                EEPROM.write(2,state);
            }
        }

        if (state == SUBINDO) {

            posicaoAtual--;

            if(posicaoAtual == 0){
                analogWrite(LPWM_Output, 0); 
                analogWrite(RPWM_Output, 0); 
                motor = 0;
                
                lcd.clear();
                lcd.setCursor(0, 0); 
                lcd.print("Varal em cima"); 
                lcd.setCursor(0, 1); 
                lcd.print("Tecle descer");
 
                state = ELEVADOR_EM_CIMA;
                EEPROM.write(0, posicaoAtual);
                EEPROM.write(2,state);
            }
        }
    } 

    
}

void calcula() {    //calcula a corrente para comparaçao do peso  
    current = 0.0;    
    for(int i =0;i<100;i++){//calculo corrente
        voltage_raw =   (5.0 / 1023.0)* analogRead(voltageSensor);// Read the voltage from sensor
        voltage =  voltage_raw - QOV + 0.012 ;// 0.000 is a value to make voltage zero when there is no current
        current = current +(voltage / 0.100);                             
    }            
    current = current/100;    

    if(current > 14){   //MUDAR PARA 14 depois
        lcd.clear();
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        motor = 0;
        
        lcd.setCursor(0,0);
        lcd.print("Sobrecarga");

        if(state == DESCIDA_SALVADA){
            lcd.setCursor(0,1);
            lcd.print("tecle descer");
        }
        else{
            state = SOBRECARGA;
            EEPROM.write(2, state);
        }
    }
}


void salva_descida(){

    if(salvandoFlag == true){

        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        motor = 0;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Descida Salva");
        lcd.setCursor(0, 1); 
        lcd.print("salvar subida");  

        posicaoFinal = posicaoAtual;
        state = DESCIDA_SALVADA;
        EEPROM.write(0, posicaoAtual);
        EEPROM.write(1, posicaoFinal);
        EEPROM.write(2, state);

    } else {

        lcd.setBacklight(HIGH); 
        lcd.clear(); 
        lcd.setCursor(0, 0);
        lcd.print("Salvando Descida"); 

        salvandoFlag = true;
        posicaoAtual = 0;

        motor = 1; 
        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 170); 
    }

}


void salva_subida(){
    
    if(salvandoFlag == true){

        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        motor = 0;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Elevador Pronto");
        lcd.setCursor(0, 1); 
        lcd.print("Tecle descer");  

        //posicaoAtual = 0;
        state = ELEVADOR_EM_CIMA;
        EEPROM.write(0, posicaoAtual);
        EEPROM.write(1, posicaoFinal);
        EEPROM.write(2, state);

    } else {

        lcd.setBacklight(HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Salvando Subida");

        posicaoAtual = EEPROM.read(0);
        posicaoFinal = EEPROM.read(1);
        salvandoFlag = true;

        analogWrite(LPWM_Output, 170);
        analogWrite(RPWM_Output, 0);
        motor = 1; 
    }

}
    

void func_descida(){

    state = DESCENDO;
    posicaoAtual = EEPROM.read(0);
    posicaoFinal = EEPROM.read(1);

    lcd.setBacklight(HIGH); 
    lcd.clear();
    lcd.setCursor(0, 1); 
    lcd.print("Varal descendo");

    analogWrite(LPWM_Output, 0); 
    analogWrite(RPWM_Output, 170); 
    motor = 1;      

}


void func_subida(){

    state = SUBINDO; 
    posicaoAtual = EEPROM.read(0);
    posicaoFinal = EEPROM.read(1);

    lcd.setBacklight(HIGH); 
    lcd.clear();
    lcd.setCursor(0, 1); 
    lcd.print("Varal Subindo");
    
    analogWrite(LPWM_Output, 170); 
    analogWrite(RPWM_Output, 0);
    motor = 1;      
}


void func_reset(){

    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("RESET");
    lcd.setCursor(0, 1); 
    lcd.print("salvar descida");

    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);

    posicaoAtual = 0;
    posicaoFinal = 0;

    EEPROM.write(0, posicaoAtual);
    EEPROM.write(1, posicaoFinal);
    wdt_reset();
}



void loop() {    

    if (onOff == 0) {       //ELEVADOR DESLIGADO
        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        lcd.setBacklight(LOW); 
        lcd.clear(); 

        if(digitalRead(BOTAO_LIGA) == HIGH){
            while(digitalRead(BOTAO_LIGA) == HIGH);
            onOff = 1;
        
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            lcd.setBacklight(HIGH); 

            if(state == ELEVADOR_EM_BAIXO){
                lcd.setCursor(0, 0); 
                lcd.print("Varal em baixo"); 
                lcd.setCursor(0, 1); 
                lcd.print("Tecle subir"); 
            }

            if(state == ELEVADOR_EM_CIMA){
                lcd.setCursor(0, 0); 
                lcd.print("Varal em cima"); 
                lcd.setCursor(0, 1); 
                lcd.print("Tecle descer"); 
            }

            if(state == FABRICA){
                lcd.setCursor(0, 1); 
                lcd.print("Tecle RESET"); 
            }

            
        }
    }

    else if(onOff == 1){    //ELEVADOR LIGADO

        switch(state){


            case FABRICA: {

                analogWrite(LPWM_Output, 0);
                analogWrite(RPWM_Output, 0);

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO; 
                    EEPROM.write(2,state);
                }

                break;
            }


            case RESETADO: {

                if(digitalRead(BOTAO_RESET) == HIGH){
                    timeCounter = millis();
                    while(digitalRead(BOTAO_RESET) == HIGH){  // RESET estado de fabrica
                        if((millis() - timeCounter) >= 5000){
                        lcd.setBacklight(HIGH); 
                        lcd.clear(); 
                        lcd.setCursor(0, 0); 
                        lcd.print("RESET FABRICA");
                        delay(2000);
                        lcd.setCursor(0, 0); 
                        lcd.print("Elevadores Harah"); 
                        lcd.setCursor(0, 1); 
                        lcd.print("Pressione Reset"); 
                        state = FABRICA;
                        EEPROM.write(2,state);
                        }
                    }
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                if(digitalRead(BOTAO_SALVA_DESCE) == HIGH){
                    while (digitalRead(BOTAO_SALVA_DESCE) == HIGH);
                    state = DESCENDO;
                    EEPROM.write(2,state);
                    salvandoFlag = false;
                    salva_descida();
                }     

                break;   
            
            }



            case DESCIDA_SALVADA: {
                
                if(digitalRead(BOTAO_SALVA_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SALVA_SOBE) == HIGH);
                    state = SUBINDO;
                    EEPROM.write(2,state);
                    salvandoFlag = false;
                    salva_subida();
                }

                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;   
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


            case SUBINDO: {
                
                if(digitalRead(BOTAO_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SOBE) == HIGH);
                    if(salvandoFlag == true){
                        salva_subida();
                    }else{
                        onOff = 0; // cancelamento de emergencia
                    }
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


            case ELEVADOR_EM_CIMA: {
                
                if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    func_descida();
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


            
            case DESCENDO: {
                
                if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    if(salvandoFlag == true){
                        salva_descida();
                    }else{
                        onOff = 0; // cancelamento de emergencia
                    }
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


            case ELEVADOR_EM_BAIXO: {
                
                if(digitalRead(BOTAO_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SOBE) == HIGH);
                    func_subida();
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


            case SOBRECARGA: {

                if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    func_descida();
                    state = ELEVADOR_EM_BAIXO;
                    EEPROM.write(2,state);
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                    EEPROM.write(2,state);
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


        }
    }
    wdt_reset();
}
