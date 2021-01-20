
// ELEVADOR DE VARAL

#define NOME ""
#define MAX_CORDA 500

#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include<LCD.h>
#include <LiquidCrystal.h> 
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
int encoder = 15;
const int voltageSensor = A0;


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
  //=============================================================== 
   
void setup() { 
    enum Verify{     // 0 = nada salvo; 1 = descida salva; 2 = ?; 3 = subida salva // subida ; 4 = desceu
    nadaSalvo, 
    descidaSalva, 
    subidaSalva, 
    elevadorEmcima, 
    elevadorEmbaixo
};  
volatile Verify verifica = nadaSalvo;

enum State{  // 0 = desligado; 1 = ligado; 2 = de fabrica;
    desligado, 
    ligado, 
    fabrica
};
volatile State estado = desligado; 
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
        lcd.print("Tecle Descer");  
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        motor = 0;
        verifica = elevadorEmcima;  // *** ver se faz sentido (se nao for elevadorEmcima sera subidaSalva)
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(2, posicaoAtual1);
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
        if(digitalRead(SALVASOBE) == HIGH){//identifica o aperto do botao, para o salvamento da subida
            while (digitalRead(SALVASOBE) == HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Subida Salva");
            lcd.setCursor(0, 1); 
            lcd.print("Programacao salva");  
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
            verifica = subidaSalva;
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
            verifica = subidaSalva;
        }                     
    }
        posicaoFinal1 = posicaoAtual1;
        posicaoFinal2 = posicaoAtual2; 
        EEPROM.write(0, posicaoFinal1);
        EEPROM.write(5,posicaoFinal2);
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(1, verifica); 
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
            lcd.setCursor(0, 0);
            lcd.print("Descida Salva");
            lcd.setCursor(0, 1); 
            lcd.print("use salvar subida");   
            analogWrite(LPWM_Output, 0); 
            analogWrite(RPWM_Output, 0); 
            motor = 0;
            verifica = descidaSalva;
        }
    }
    EEPROM.write(1, verifica);
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

    verifica = elevadorEmbaixo;

    posicaoAtual1 = 0;
    posicaoAtual2 = 0;
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
    EEPROM.write(1, verifica);
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
        verifica = subidaSalva; 
    }

    posicaoAtual1 = posicaoFinal1;
    posicaoAtual2 = posicaoFinal2;
    EEPROM.write(2, posicaoAtual1);
    EEPROM.write(3, posicaoAtual2);
    EEPROM.write(1, verifica);
}






void loop() {    

// ----- SALVAMENTO ------ 

//de Fabrica
    if(estado == fabrica && digitalRead(LIGA) == HIGH) {
        while (digitalRead(LIGA) == HIGH); 
        lcd.setBacklight(HIGH); 
        lcd.clear(); 
        lcd.setCursor(0, 0); 
        lcd.print("Configurar Elevador"); 
        lcd.setCursor(0, 1); 
        lcd.print("Pressione reset"); 
        estado = ligado;
        verifica = nadaSalvo;
    }


//salva subida
    if (digitalRead(SALVASOBE) == HIGH && estado==ligado && verifica==descidaSalva){
        while (digitalRead(SALVASOBE) == HIGH);
        salva_subida();
    }

//salva descida
    else if (digitalRead(SALVADESCE) == HIGH && estado==ligado && verifica==nadaSalvo){         
        while (digitalRead(SALVADESCE) == HIGH);
        salva_descida();
    }
    
// ----- liga, desliga e reset ------ 
 
//LIGA
    else if (estado == desligado && digitalRead(LIGA) == HIGH) {
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
        estado = ligado; 
    }

//DESLIGA
    else if (estado == ligado && digitalRead(LIGA) == HIGH) {
        while (digitalRead(LIGA) == HIGH); 

        analogWrite(LPWM_Output, 0); 
        analogWrite(RPWM_Output, 0); 
        lcd.setBacklight(LOW); 
        lcd.clear(); 
        estado = desligado;
    }

//RESET
    else if(digitalRead(RESET) == HIGH){
        timeCounter = 0;

        while(digitalRead(RESET) == HIGH){  // RESET estado de fabrica
            if((timeCounter - millis()) >= 5000){
            lcd.setBacklight(HIGH); 
            lcd.clear(); 
            lcd.setCursor(0, 0); 
            lcd.print("RESET FABRICA");
            delay(2000);
            lcd.setBacklight(LOW);
            estado = fabrica;
            }
        }

        if(estado != fabrica){ // se for reset normal
            lcd.setBacklight(HIGH); 
            lcd.clear(); 
            lcd.setCursor(0, 0); 
            lcd.print("RESET");
            estado = ligado;
            verifica = nadaSalvo;
            lcd.setCursor(1, 0); 
            lcd.print("use salvar descida");
        }
        

        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);

        posicaoFinal1 = 0;
        posicaoFinal2 = 0;
        posicaoAtual1 = 0;
        posicaoAtual2 = 0;
        verifica = nadaSalvo;

        EEPROM.write(0, posicaoFinal1);
        EEPROM.write(2, posicaoAtual1);
        EEPROM.write(3, posicaoAtual2);
        EEPROM.write(5, posicaoFinal2);
        EEPROM.write(1, verifica);

    }        

// ----- sobe e desce ------ 

//Subida     
    if(  (digitalRead(SOBE) == HIGH && estado==ligado && verifica==elevadorEmbaixo)   ||   ((digitalRead(SOBE) == HIGH && estado==ligado &&  EEPROM.read(2) == 0 && EEPROM.read(3) == 0) && verifica!=descidaSalva && verifica!=subidaSalva && EEPROM.read(1)==0)  ) {        
        while (digitalRead(SOBE) == HIGH);
        func_subida();
    }

//Descida
    else if(  (digitalRead(DESCE) == HIGH && estado==ligado && verifica==subidaSalva)   ||   ((digitalRead(DESCE) == HIGH && estado==ligado && ( EEPROM.read(2)+EEPROM.read(3)*255) == (EEPROM.read(0)+EEPROM.read(5)*255) ) && verifica!=descidaSalva && verifica!=subidaSalva && EEPROM.read(1)==0 )  ) {          
        while (digitalRead(DESCE) == HIGH);
        func_descida();
    }
              
}
