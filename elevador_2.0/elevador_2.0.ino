
// ELEVADOR DE VARAL

//=============================================================== 


#define NOME ""
#define MAX_CORDA 500

//BIBLIOTECAS
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include<LCD.h>
#include <LiquidCrystal.h> 
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

//ESTADOS
#define FABRICA 0
#define RESETADO 1
#define DESCIDA_SALVADA 2
//#define SUBIDA_SALVADA 3
#define ELEVADOR_EM_CIMA 4
#define ELEVADOR_EM_BAIXO 5
#define SOBRECARGA 6
#define CORDA_NO_LIMITE 7

//PINOS
int RPWM_Output = 5; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM) 
int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM) 
int BOTAO_SOBE = 3; 
int BOTAO_LIGA = 2; 
int BOTAO_DESCE = 4; 
int BOTAO_RESET = 10; 
int BOTAO_SALVA_SOBE = 11;
int BOTAO_SALVA_DESCE = 12;
int encoder = 15;
const int voltageSensor = A0;


//VARIAVEIS
int motor = 0;   
int posicaoAtual1 = 0;
int posicaoAtual2 = 0;
int posicaoFinal1 = 0;
int posicaoFinal2 = 0;
const float VCC   = 5.0;// supply voltage is from 4.5 to 5.5V. Normally 5V.
const float QOV =   0.5 * VCC;// set quiescent Output voltage of 0.5V.
float voltage;// internal variable for voltage  
float current=0;
float voltage_raw=0;
unsigned long timeCounter = 0;


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
    lcd.setCursor(0, 0); 
    lcd.print("Elevadores Harah"); 
    lcd.setCursor(0, 1); 
    lcd.print("Pressione Reset"); 
    state = FABRICA;
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
}




void calcula() {//calcula a corrente para comparaçao do peso  
    current = 0.0;    
    for(int i =0;i<100;i++){//calculo corrente
      voltage_raw =   (5.0 / 1023.0)* analogRead(voltageSensor);// Read the voltage from sensor
      voltage =  voltage_raw - QOV + 0.012 ;// 0.000 is a value to make voltage zero when there is no current
      current = current +(voltage / 0.100);                             
    }            
    current = current/100;    

    if(current > 14){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Sobrecarga");
        lcd.setCursor(0, 1); 
        lcd.print("Tecle botao descer");  
        state = SOBRECARGA;
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        motor = 0;
        EEPROM.write(0, posicaoAtual1);
        EEPROM.write(1, posicaoAtual2);
    }
}


void salva_descida(){
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print("Salvando Descida"); 
    analogWrite(LPWM_Output, 0); 
    analogWrite(RPWM_Output, 170); 
    motor = 1; 

    while(motor){ 
        if(digitalRead(BOTAO_SALVA_DESCE) == HIGH ){
            while (digitalRead(BOTAO_SALVA_DESCE) == HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Descida Salva");
            lcd.setCursor(0, 1); 
            lcd.print("use salvar subida");   
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
        }
    }
}


void salva_subida(){
    lcd.setBacklight(HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Salvando Subida");
    analogWrite(LPWM_Output, 190);
    analogWrite(RPWM_Output, 0);
    motor = 1;
    posicaoAtual1 = 0;
    posicaoAtual2 = 0;  

    while(motor){       
        calcula();     
       
        if(digitalRead(encoder)==HIGH){//faz a contagem de quantas voltas foram feitas na polia
            while (digitalRead(encoder) == HIGH); 
            if(posicaoAtual1 == 255){
                posicaoAtual1 = 0;
                posicaoAtual2++;
            }
            else{
                posicaoAtual1++;
            }

            posicaoFinal1 = posicaoAtual1;
            posicaoFinal2 = posicaoAtual2; 
            EEPROM.write(0, posicaoAtual1);
            EEPROM.write(1, posicaoAtual2);
            EEPROM.write(2, posicaoFinal1);
            EEPROM.write(3,posicaoFinal2);
        }

        if(digitalRead(BOTAO_SALVA_SOBE) == HIGH){//identifica o aperto do botao, para o salvamento da subida
            while (digitalRead(BOTAO_SALVA_SOBE) == HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Subida Salva");
            lcd.setCursor(0, 1); 
            lcd.print("Programacao salva");  
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
        }
        
        if((posicaoAtual1 + posicaoAtual2*255) == MAX_CORDA){
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Chegou ao Maximo");
            lcd.setCursor(0, 1); 
            lcd.print("Programacao salva");  
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
            state = CORDA_NO_LIMITE;
        }                     
    }
}
    

void func_descida(){
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Varal descendo");
    lcd.setCursor(0, 1); 
    lcd.print(NOME);
    analogWrite(LPWM_Output, 0); 
    analogWrite(RPWM_Output, 170); 
    posicaoAtual1 = EEPROM.read(0);
    posicaoAtual2 = EEPROM.read(1);
    posicaoFinal1 = EEPROM.read(2);
    posicaoFinal2 = EEPROM.read(3);

    int posTotal = 255*posicaoAtual2 + posicaoAtual1; 
    motor = 1;   

    while(motor){     
        calcula(); 
        if(digitalRead(encoder)==HIGH){ 
            while (digitalRead(encoder) == HIGH);
            if(posTotal > 0){
                posTotal--;  
            }
        }
        if(posTotal == 0){
            lcd.clear(); 
            lcd.print("Chegou embaixo"); 
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
        }
    }

    posicaoAtual1 = 0;
    posicaoAtual2 = 0;
    EEPROM.write(0, posicaoAtual1);
    EEPROM.write(1, posicaoAtual2);
}



void func_subida(){
    analogWrite(LPWM_Output, 190); 
    analogWrite(RPWM_Output, 0); 
    posicaoAtual1 = EEPROM.read(0);
    posicaoAtual2 = EEPROM.read(1);
    posicaoFinal1 = EEPROM.read(2);
    posicaoFinal2 = EEPROM.read(3);

    int posFinalTotal = 0;
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 1); 
    lcd.print("Varal Subindo");

    motor = 1;      

    while(motor){
        calcula();
        //lcd.setCursor(10, 1);
        //lcd.print(current);              
        if(digitalRead(encoder)==HIGH){ 
            while (digitalRead(encoder) == HIGH);
            posFinalTotal++;
            //lcd.setCursor(0, 1); 
            //lcd.print(posFinalTotal);
        }            
        if(posFinalTotal == posicaoFinal1 + posicaoFinal2*255){
            lcd.clear(); 
            lcd.print("Chegou no topo"); 
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
        }
    }

    posicaoAtual1 = posicaoFinal1;
    posicaoAtual2 = posicaoFinal2;
    EEPROM.write(0, posicaoAtual1);
    EEPROM.write(1, posicaoAtual2);
}

void func_reset(){
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("RESET");
    lcd.setCursor(0, 1); 
    lcd.print("use salvar descida");

    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);

    posicaoAtual1 = 0;
    posicaoAtual2 = 0;
    posicaoFinal1 = 0;
    posicaoFinal2 = 0;

    EEPROM.write(0, posicaoAtual1);
    EEPROM.write(1, posicaoAtual2);
    EEPROM.write(2, posicaoFinal1);
    EEPROM.write(3, posicaoFinal2);
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
            lcd.clear(); 
            lcd.setCursor(0, 0); 
            lcd.print("Elevadores Harah"); 

            
        }
    }

    else if(onOff == 1){    //ELEVADOR LIGADO

        switch(state){


            case FABRICA: {

                    analogWrite(LPWM_Output, 0);
                    analogWrite(RPWM_Output, 0);

                    posicaoFinal1 = 0;
                    posicaoFinal2 = 0;
                    posicaoAtual1 = 0;
                    posicaoAtual2 = 0;

                    EEPROM.write(0, posicaoAtual1);
                    EEPROM.write(1, posicaoAtual2);
                    EEPROM.write(2, posicaoFinal1);
                    EEPROM.write(3, posicaoFinal2);

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;   
                }

                break;
            }


            case RESETADO: {

                timeCounter = 0;
                while(digitalRead(BOTAO_RESET) == HIGH){  // RESET estado de fabrica
                    if((timeCounter - millis()) >= 5000){
                    lcd.setBacklight(HIGH); 
                    lcd.clear(); 
                    lcd.setCursor(0, 0); 
                    lcd.print("RESET FABRICA");
                    delay(2000);
                    lcd.setBacklight(LOW);
                    lcd.setCursor(0, 0); 
                    lcd.print("Elevadores Harah"); 
                    lcd.setCursor(0, 1); 
                    lcd.print("Pressione Reset"); 
                    state = FABRICA;
                    }
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                if(digitalRead(BOTAO_SALVA_DESCE) == HIGH){
                    while (digitalRead(BOTAO_SALVA_DESCE) == HIGH);
                    salva_descida();
                    state = DESCIDA_SALVADA;
                }     

                break;   
            
            }


            case DESCIDA_SALVADA: {
                
                if(digitalRead(BOTAO_SALVA_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SALVA_SOBE) == HIGH);
                    salva_subida();
                    state = ELEVADOR_EM_CIMA;
                }

                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;   
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
                    state = ELEVADOR_EM_BAIXO;
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
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
                    state = ELEVADOR_EM_CIMA;
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
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
                }
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    func_reset();
                    state = RESETADO;
                }

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }

            
            case CORDA_NO_LIMITE: {

                if(digitalRead(BOTAO_LIGA) == HIGH){
                    while(digitalRead(BOTAO_LIGA) == HIGH);
                    onOff = 0;
                }

                break;
            }


        }
    }

}