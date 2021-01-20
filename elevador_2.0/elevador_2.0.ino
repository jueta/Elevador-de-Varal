
// ELEVADOR DE VARAL

//=============================================================== 


#define NOME ""
#define MAX_CORDA 500

//BIBLIOTECAS
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); 

//ESTADOS
#define FABRICA 0
#define RESET 1
#define SALVA_DESCIDA 2
#define SALVA_SUBIDA 3
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
int posicao;
int motor = 0;   
int posicaoFinal1 = 0;
int posicaoFinal2 = 0;
int posicaoAtual1 = EEPROM.read(2);
int posicaoAtual2 = EEPROM.read(3);
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
    posicao = 1;
    state = FABRICA;
    onOff = 0;
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
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        motor = 0;
        posicao = 1;
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(1, posicao);
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
            lcd.setCursor(0, 1); 
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
            posicao = 0;
        }                     
    }
        posicaoFinal1 = posicaoAtual1;
        posicaoFinal2 = posicaoAtual2; 
        EEPROM.write(0, posicaoFinal1);
        EEPROM.write(5,posicaoFinal2);
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(1, posicao); 
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

void func_descida(){
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Varal descendo");
    lcd.setCursor(0, 1); 
    lcd.print(NOME);
    analogWrite(LPWM_Output, 0); 
    analogWrite(RPWM_Output, 170); 
    posicaoFinal1 = EEPROM.read(0);
    posicaoFinal2 = EEPROM.read(5);

    int posTotal = 255*posicaoFinal2 + posicaoFinal1; 
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
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
}

void func_subida(){
    analogWrite(LPWM_Output, 190); 
    analogWrite(RPWM_Output, 0); 
    posicaoFinal1 = EEPROM.read(0);
    posicaoFinal2 = EEPROM.read(5);

    int posFinalTotal = 0;
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Varal Subindo");
    lcd.setCursor(0, 1); 
    lcd.print(NOME); 

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
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
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
        
            EEPROM.write(1, 0);
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            lcd.setBacklight(HIGH); 
            lcd.clear(); 
            lcd.setCursor(0, 0); 
            lcd.print("BOTAO_LIGAdo"); 
            lcd.setCursor(0, 1); 
            lcd.print("Elevadores Harah"); 
            delay (2000);
        }
    }

    else if(onOff == 1){    //ELEVADOR LIGADO

        switch(state){


            case FABRICA: {

                lcd.setBacklight(HIGH); 
                lcd.clear(); 
                lcd.setCursor(0, 0); 
                lcd.print("Configurar Elevador"); 
                lcd.setCursor(0, 1); 
                lcd.print("Pressione reset"); 

                analogWrite(LPWM_Output, 0);
                analogWrite(RPWM_Output, 0);

                posicaoFinal1 = 0;
                posicaoFinal2 = 0;
                posicaoAtual1 = 0;
                posicaoAtual2 = 0;

                EEPROM.write(0, posicaoFinal1);
                EEPROM.write(2, posicaoAtual1);
                EEPROM.write(3, posicaoAtual2);
                EEPROM.write(5, posicaoFinal2);
                //EEPROM.write(1, verifica);
                
                if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    state = RESET;       
                }

                break;
            }


            case RESET: {

                timeCounter = 0;
                while(digitalRead(BOTAO_RESET) == HIGH){  // RESET estado de fabrica
                    if((timeCounter - millis()) >= 5000){
                    lcd.setBacklight(HIGH); 
                    lcd.clear(); 
                    lcd.setCursor(0, 0); 
                    lcd.print("RESET FABRICA");
                    delay(2000);
                    lcd.setBacklight(LOW);
                    state = FABRICA;
                    }
                }

                if(state != FABRICA){ // se for reset normal
                    lcd.setBacklight(HIGH); 
                    lcd.clear(); 
                    lcd.setCursor(0, 0); 
                    lcd.print("RESET");
                    lcd.setCursor(1, 0); 
                    lcd.print("use salvar descida");

                    analogWrite(LPWM_Output, 0);
                    analogWrite(RPWM_Output, 0);

                    posicao = 1;
                    posicaoFinal1 = 0;
                    posicaoFinal2 = 0;
                    posicaoAtual1 = 0;
                    posicaoAtual2 = 0;

                    EEPROM.write(0, posicaoFinal1);
                    EEPROM.write(2, posicaoAtual1);
                    EEPROM.write(3, posicaoAtual2);
                    EEPROM.write(5, posicaoFinal2);
                    //EEPROM.write(1, verifica);
                }

                if(digitalRead(BOTAO_SALVA_DESCE) == HIGH){
                    while (digitalRead(BOTAO_SALVA_DESCE) == HIGH);
                    state = SALVA_DESCIDA;
                    
                }     

                break;   
            
            }


            case SALVA_DESCIDA: {

                if(posicao == 1){
                    salva_descida();
                    posicao = 0;
                }
                else if(digitalRead(BOTAO_SALVA_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SALVA_SOBE) == HIGH);
                    state = SALVA_SUBIDA;
                }

                break;
            }


            case SALVA_SUBIDA: {

                if(posicao == 0){
                    salva_subida();
                    posicao = 1;
                }
                else if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    state = ELEVADOR_EM_BAIXO;
                }
                else if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    state = RESET;
                }

                break;
            }

            
            case ELEVADOR_EM_BAIXO: {

                if(posicao == 1){
                    func_descida();
                    posicao = 0;
                }
                else if(digitalRead(BOTAO_SOBE) == HIGH){
                    while(digitalRead(BOTAO_SOBE) == HIGH);
                    state = ELEVADOR_EM_CIMA;
                }
                else if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    state = RESET;
                }

                break;
            }

                
            case ELEVADOR_EM_CIMA: {

                if(posicao == 0){
                    func_subida();
                    posicao = 1;
                }
                else if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    state = ELEVADOR_EM_BAIXO;
                }
                else if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    state = RESET;
                }

                break;
            }


            case SOBRECARGA: {

                if(digitalRead(BOTAO_DESCE) == HIGH){
                    while(digitalRead(BOTAO_DESCE) == HIGH);
                    state = ELEVADOR_EM_CIMA;
                    posicao = 1;
                }
                else if(digitalRead(BOTAO_RESET) == HIGH){
                    while(digitalRead(BOTAO_RESET) == HIGH);
                    state = RESET;
                }

                break;
            }

            
            case CORDA_NO_LIMITE: {

                break;
            }


        }
    }

}
