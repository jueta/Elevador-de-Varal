
// ELEVADOR DE VARAL

#define NOME ""

#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include<LCD.h> 
#include <EEPROM.h>

  LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 
  
  int RPWM_Output = 5; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM) 
  int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM) 
  int SOBE = 3;//Porta que comando o sentido horario 
  int LIGA = 2; 
  int DESCE = 4; //Porta que comanda o sentido anti-horario 
  int RESET = 10; 
  int SALVASOBE = 11;
  int SALVADESCE = 12;
  int sensor = 15;
  int buttonPin = 9; // número do pino pushbutton   
  int verificadescida = 0;
 
  int estado = 0;
  int motor = 0;   
  int sensorcorrente;   
  int posicaoFinal1 = 0;
  int posicaoFinal2 = 0;
  int posicaoAtual1 = EEPROM.read(2);
  int posicaoAtual2 = EEPROM.read(3);
  const int pinoSensor = A0;
  const float VCC   = 5.0;// supply voltage is from 4.5 to 5.5V. Normally 5V.
  const float QOV =   0.5 * VCC;// set quiescent Output voltage of 0.5V
  float voltage;// internal variable for voltage  
  float current=0;
  float voltage_raw=0;
  int salvamento = 0;
  //=============================================================== 
   
void setup() { 
    Serial.begin(9600); 
    lcd.begin(16, 2);     
    lcd.clear(); 
    lcd.setCursor(3, 0);
    lcd.setBacklight(LOW); 
    pinMode(RPWM_Output, OUTPUT); 
    pinMode(LPWM_Output, OUTPUT); 
    pinMode(SOBE, INPUT); 
    pinMode(LIGA, INPUT); 
    pinMode(DESCE, INPUT); 
    pinMode(RESET, INPUT); 
    pinMode(SALVASOBE, INPUT); 
    pinMode(SALVADESCE, INPUT); 
    pinMode(pinoSensor,INPUT); 
    pinMode(sensor,INPUT);
}

void calcula() {//calcula a corrente para comparaçao do peso  
    current = 0.0;    
    for(int i =0;i<100;i++){//calculo corrente
      voltage_raw =   (5.0 / 1023.0)* analogRead(pinoSensor);// Read the voltage from sensor
      voltage =  voltage_raw - QOV + 0.012 ;// 0.000 is a value to make voltage zero when there is no current
      current = current +(voltage / 0.100);                             
    }            
    current = current/100;    

    if(current > 14){
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Sobrecarga");
       analogWrite(LPWM_Output, 0);
       analogWrite(RPWM_Output, 0);
       motor = 0;
       verificadescida = 3;  
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
        if(digitalRead(sensor)==HIGH){//faz a contagem de quantas voltas foram feitas na polia
            while (digitalRead(sensor) == HIGH); 
            if(posicaoAtual1 == 255){
                posicaoAtual1 = 0;
                posicaoAtual2++;
            }
            else{
                posicaoAtual1++;
            }
            lcd.setCursor(0, 1); 
        }
        if(digitalRead(SALVASOBE) == HIGH ){//identifica o aperto do botao, para o salvamento da subida
            while (digitalRead(SALVASOBE) == HIGH);
            lcd.clear();
            lcd.print("Subida Salva"); 
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
            verificadescida = 3; 
        }                          
    }
        posicaoFinal1 = posicaoAtual1;
        posicaoFinal2 = posicaoAtual2; 
        EEPROM.write(0, posicaoFinal1);
        EEPROM.write(5,posicaoFinal2);
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(1, verificadescida); 
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
        if(digitalRead(SALVADESCE) == HIGH ){
            while (digitalRead(SALVADESCE) == HIGH);
            lcd.clear();
            lcd.print("Descida Salva");    
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
            verificadescida = 1;
        }
    }
    EEPROM.write(1, verificadescida);
}

void func_descida(){
    lcd.setBacklight(HIGH); 
    lcd.clear(); 
    lcd.setCursor(0, 0); 
    lcd.print("Varal Descendo");

    lcd.setCursor(0, 1); 
    lcd.print(NOME);
    analogWrite(LPWM_Output, 0); 
    analogWrite(RPWM_Output, 170); 
    posicaoFinal1 = EEPROM.read(0);
    posicaoFinal2 = EEPROM.read(5);
    lcd.setCursor(0, 0); 
    lcd.print("Varal Descendo");

    lcd.setCursor(0, 1); 
    lcd.print(NOME);
    int posTotal = 255*posicaoFinal2 + posicaoFinal1; 
    motor = 1;   

    while(motor){     
        calcula(); 
        if(digitalRead(sensor)==HIGH){ 
            while (digitalRead(sensor) == HIGH);
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

    verificadescida=4;
    posicaoAtual1 = 0;
    posicaoAtual2 = 0;
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
    EEPROM.write(1, verificadescida);
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
        if(digitalRead(sensor)==HIGH){ 
            while (digitalRead(sensor) == HIGH);
            posFinalTotal++;
            //lcd.setCursor(0, 1); 
            //lcd.print(posFinalTotal);
        }            
        if(posFinalTotal == posicaoFinal1 + posicaoFinal2*255){
            lcd.clear(); 
            lcd.print("Chegou no topo"); 
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;}
            verificadescida=3;
        }

    posicaoAtual1 = posicaoFinal1;
    posicaoAtual2 = posicaoFinal2;
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
    EEPROM.write(1, verificadescida);
}






void loop() {    

// ----- SALVAMENTO ------ 

//salva subida
    if (digitalRead(SALVASOBE) == HIGH && estado==1 && verificadescida==1){
        while (digitalRead(SALVASOBE) == HIGH);
        salva_subida();
    }

//salva descida
    else if (digitalRead(SALVADESCE) == HIGH && estado==1 && verificadescida==0){         
        while (digitalRead(SALVADESCE) == HIGH);
        salva_descida();
    }
    
// ----- liga, desliga e reset ------ 
 
//LIGA
    else if ( estado == 0 && digitalRead(LIGA) == HIGH ) {
        while (digitalRead(LIGA) == HIGH); 
        
        EEPROM.write(1, 0);
        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        lcd.setBacklight(HIGH); 
        lcd.clear(); 
        lcd.setCursor(0, 0); 
        lcd.print("Ligado"); 
        lcd.setCursor(0, 1); 
        lcd.print("Elevadores Harah"); 
        estado = 1; 
    }

//DESLIGA
    else if ( estado == 1 && digitalRead(LIGA) == HIGH ) {
        while (digitalRead(LIGA) == HIGH); 

        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        lcd.setBacklight(LOW); 
        lcd.clear(); 
        estado = 0;
    }

//RESET
    else if(estado == 1 && digitalRead(RESET) == HIGH){
        while(digitalRead(RESET) == HIGH);

        lcd.setBacklight(HIGH); 
        lcd.clear(); 
        lcd.setCursor(0, 0); 
        lcd.print("RESET");
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);

        posicaoFinal1 = 0;
        posicaoFinal2 = 0;
        posicaoAtual1 = 0;
        posicaoAtual2 = 0;
        verificadescida = 0;
        salvamento = 1;

        EEPROM.write(0, posicaoFinal1);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(5, posicaoFinal2);
        EEPROM.write(1, verificadescida);
    }        

// ----- sobe e desce ------ 

//Subida     
    if ((digitalRead(SOBE) == HIGH && estado==1 && verificadescida==4) || ((digitalRead(SOBE) == HIGH && estado==1 &&  EEPROM.read(2) == 0 && EEPROM.read(3) == 0) && verificadescida!=1 && verificadescida!=3 && EEPROM.read(1)==0)) {        
        while (digitalRead(SOBE) == HIGH);
        func_subida();
    }

//Descida
    else if ((digitalRead(DESCE) == HIGH && estado==1 && verificadescida==3) || ((digitalRead(DESCE) == HIGH && estado==1 &&  (EEPROM.read(2)+EEPROM.read(3)*255) == (EEPROM.read(0)+EEPROM.read(5)*255)) && verificadescida!=1 && verificadescida!=3 && EEPROM.read(1)==0 )) {          
        while (digitalRead(DESCE) == HIGH);
        func_descida();
    }
              
}
