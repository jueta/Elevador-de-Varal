
// Elevador DE VARAL

//===============================================================

#define MAX_CORDA 120

// BIBLIOTECAS
#include <Arduino.h>
#include <elevadorFunctions.h>

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// PINOS
const int RPWM_Output = 5; // Arduino PWM output pin 5; connect to IBT-2 pin 1 (RPWM)
const int LPWM_Output = 6; // Arduino PWM output pin 6; connect to IBT-2 pin 2 (LPWM)
const int BOTAO_SOBE = 7;
const int BOTAO_LIGA = 3;
const int BOTAO_DESCE = 4;
const int BOTAO_RESET = 10;
const int BOTAO_SALVA_SOBE = 11;
const int BOTAO_SALVA_DESCE = 12;
const int encoder = 2; // Mudar para pino 2 ou 3
const int currentSensor = A0;
const int powerOffSensor = A2;

// VARIAVEIS
static volatile unsigned long debounce = 0;
const float VCC = 5.0;       // supply voltage is from 4.5 to 5.5V. Normally 5V.
const float QOV = 0.5 * VCC; // set quiescent Output voltage of 0.5V.
float voltage;               // internal variable for voltage
float voltage_raw = 0;
unsigned long timeCounter;
int aux = 0;
bool flagFim = false;
unsigned long currTimer = 0;

Varal Elevador;

void setup()
{

    Serial.begin(9600);

    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.setBacklight(LOW);

    Elevador.onOff = 0;

    Elevador.posicaoAtual = EEPROM.read(0);
    Elevador.posicaoFinal = EEPROM.read(1);
    Elevador.state = EEPROM.read(2);

    pinMode(RPWM_Output, OUTPUT);
    pinMode(LPWM_Output, OUTPUT);
    pinMode(BOTAO_SOBE, INPUT);
    pinMode(BOTAO_LIGA, INPUT);
    pinMode(BOTAO_DESCE, INPUT);
    pinMode(BOTAO_RESET, INPUT);
    pinMode(BOTAO_SALVA_SOBE, INPUT);
    pinMode(BOTAO_SALVA_DESCE, INPUT);
    pinMode(currentSensor, INPUT);
    pinMode(encoder, INPUT);

    Serial.begin(9600);

    wdt_enable(WDTO_8S);

    attachInterrupt(digitalPinToInterrupt(encoder), holeCounter, FALLING);

    MsTimer2::set(10, BrownOutDetect); // 1ms period
    MsTimer2::start();

    Serial.print("Ligando \n");
}

void loop()
{

    if (Elevador.onOff == 0)
    { // Elevador DESLIGADO
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        lcd.setBacklight(LOW);
        lcd.clear();

        if (digitalRead(BOTAO_LIGA) == HIGH)
        {
            while (digitalRead(BOTAO_LIGA) == HIGH)
                ;
            Elevador.onOff = 1;

            analogWrite(LPWM_Output, 0);
            analogWrite(RPWM_Output, 0);
            lcd.setBacklight(HIGH);

            if (Elevador.state == ELEVADOR_EM_BAIXO)
            {
                lcd.setCursor(0, 0);
                lcd.print("Varal em baixo");
                Serial.print("Varal em baixo \n");
                Serial.print("Elevador.posicaoAtual = ");
                Serial.println(Elevador.posicaoAtual);
                lcd.setCursor(0, 1);
                lcd.print("Tecle subir");
            }

            if (Elevador.state == ELEVADOR_EM_CIMA)
            {
                lcd.setCursor(0, 0);
                lcd.print("Varal em cima");
                Serial.print("Varal em cima \n");
                Serial.print("Elevador.posicaoAtual = ");
                Serial.println(Elevador.posicaoAtual);
                lcd.setCursor(0, 1);
                lcd.print("Tecle descer");
            }

            if (Elevador.state == FABRICA)
            {
                lcd.setCursor(0, 1);
                lcd.print("Tecle RESET");
            }
        }
    }

    else if (Elevador.onOff == 1)
    { // Elevador LIGADO

        switch (Elevador.state)
        {

        case FABRICA:
        {

            analogWrite(LPWM_Output, 0);
            analogWrite(RPWM_Output, 0);

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            break;
        }

        case RESETADO:
        {

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                timeCounter = millis();
                while (digitalRead(BOTAO_RESET) == HIGH)
                { // RESET estado de fabrica
                    if ((millis() - timeCounter) >= 5000)
                    {
                        lcd.setBacklight(HIGH);
                        lcd.clear();
                        lcd.setCursor(0, 0);
                        lcd.print("RESET FABRICA");
                        delay(2000);
                        lcd.setCursor(0, 0);
                        lcd.print("Elevadores Harah");
                        lcd.setCursor(0, 1);
                        lcd.print("Pressione Reset");
                        Elevador.state = FABRICA;
                        EEPROM.write(2, Elevador.state);
                    }
                }
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            if (digitalRead(BOTAO_SALVA_DESCE) == HIGH)
            {
                while (digitalRead(BOTAO_SALVA_DESCE) == HIGH)
                    ;
                Elevador.salvandoFlag = false;
                salva_descida();
            }

            break;
        }

        case DESCIDA_SALVADA:
        {

            if (digitalRead(BOTAO_SALVA_SOBE) == HIGH)
            {
                while (digitalRead(BOTAO_SALVA_SOBE) == HIGH)
                    ;
                Elevador.state = SUBINDO;
                EEPROM.write(2, Elevador.state);
                Elevador.salvandoFlag = false;
                salva_subida();
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }

        case SUBINDO:
        {

            // calcula();

            // if(digitalRead(BOTAO_SOBE) == HIGH){ // cancelamento de emergencia
            //     while(digitalRead(BOTAO_SOBE) == HIGH);
            //         Elevador.onOff = 0;
            // }

            if (Elevador.salvandoFlag == true && digitalRead(BOTAO_SALVA_SOBE) == HIGH)
            {
                while (digitalRead(BOTAO_SALVA_SOBE) == HIGH)
                    ;
                salva_subida();
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }

        case ELEVADOR_EM_CIMA:
        {

            if (digitalRead(BOTAO_DESCE) == HIGH)
            {
                while (digitalRead(BOTAO_DESCE) == HIGH)
                    ;
                func_descida();
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }

        case DESCENDO:
        {

            calcula();

            // if(digitalRead(BOTAO_DESCE) == HIGH){ // cancelamento de emergencia
            //     while(digitalRead(BOTAO_DESCE) == HIGH);
            //         Elevador.onOff = 0; 
            // }

            if (Elevador.salvandoFlag == true && digitalRead(BOTAO_SALVA_DESCE) == HIGH)
            {
                while (digitalRead(BOTAO_SALVA_DESCE) == HIGH)
                    ;
                salva_descida();
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }

        case ELEVADOR_EM_BAIXO:
        {

            if (digitalRead(BOTAO_SOBE) == HIGH)
            {
                while (digitalRead(BOTAO_SOBE) == HIGH)
                    ;
                func_subida();
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }

        case SOBRECARGA:
        {

            if (digitalRead(BOTAO_DESCE) == HIGH)
            {
                while (digitalRead(BOTAO_DESCE) == HIGH)
                    ;
                func_descida();
                Elevador.state = ELEVADOR_EM_BAIXO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_RESET) == HIGH)
            {
                while (digitalRead(BOTAO_RESET) == HIGH)
                    ;
                func_reset();
                Elevador.state = RESETADO;
                EEPROM.write(2, Elevador.state);
            }

            if (digitalRead(BOTAO_LIGA) == HIGH)
            {
                while (digitalRead(BOTAO_LIGA) == HIGH)
                    ;
                Elevador.onOff = 0;
            }

            break;
        }
        }
    }
    wdt_reset();
}

/****************************************************************************
 *
 *  FUNCOES
 *
 ****************************************************************************/

void fimDescida()
{

    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);
    Elevador.motor = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Varal embaixo");
    lcd.setCursor(0, 1);
    lcd.print("Tecle subir");
    Serial.print("Fim descida \n");
    Serial.print("Elevador.posicalAtual = ");
    Serial.println(Elevador.posicaoAtual);

    Elevador.state = ELEVADOR_EM_BAIXO;
}

void fimSubida()
{
    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);
    Elevador.motor = 0;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Varal em cima");
    lcd.setCursor(0, 1);
    lcd.print("Tecle descer");
    Serial.print("Fim subida \n");
    Serial.print("Elevador.posicalAtual = ");
    Serial.println(Elevador.posicaoAtual);

    Elevador.state = ELEVADOR_EM_CIMA;
}

void BrownOutDetect()
{ // detecta a queda de tensao no pino A3 e salva tudo

    if (analogRead(powerOffSensor) < 920)
    {

        pinMode(powerOffSensor, OUTPUT);
        digitalWrite(powerOffSensor, HIGH);
        return;
    }

    pinMode(powerOffSensor, INPUT);

    if (analogRead(powerOffSensor) > 1000)
    {
        Elevador.state = ELEVADOR_EM_CIMA;
        EEPROM.write(0, Elevador.posicaoAtual); // Salva a posicao que parou
        EEPROM.write(1, Elevador.posicaoFinal);
        EEPROM.write(2, Elevador.state);
        Serial.print("BrownOut Detected! \n");
        Serial.print("Saved! \n");

    }
}

void holeCounter()
{ // Contador de furos do Encoder   **ISR**

    if ((millis() - debounce) > 20)
    {

        // while(digitalRead (encoder));

        debounce = millis();

        if (Elevador.salvandoFlag == true)
        { // Is saving

            if (Elevador.state == DESCENDO)
            {

                // Elevador.auxMaxCorda++;

                // if(Elevador.auxMaxCorda == MAX_CORDA){
                //     analogWrite(LPWM_Output, 0);
                //     analogWrite(RPWM_Output, 0);
                //     Elevador.motor = 0;

                //     lcd.clear();
                //     lcd.setCursor(0, 0);
                //     lcd.print("Chegou ao Maximo");
                //     lcd.setCursor(0, 1);
                //     lcd.print("Descida salva");

                //     Elevador.posicaoFinal = Elevador.posicaoAtual;
                //     EEPROM.write(0, Elevador.posicaoAtual); // Salva a posicao que parou
                //     EEPROM.write(1, Elevador.posicaoFinal);
                //     EEPROM.write(2,Elevador.state);
                // }
            }

            else if (Elevador.state == SUBINDO)
            {

                Elevador.posicaoAtual++;
            }
        }
        else
        { // Normal Function

            if (Elevador.state == DESCENDO || Elevador.state == ELEVADOR_EM_BAIXO)
            {

                Elevador.posicaoAtual--;

                if (Elevador.posicaoAtual <= 0)
                {
                    fimDescida();
                }
            }

            else if (Elevador.state == SUBINDO || Elevador.state == ELEVADOR_EM_CIMA)
            {

                Elevador.posicaoAtual++;

                if (Elevador.posicaoAtual >= Elevador.posicaoFinal)
                {
                    fimSubida();
                }
            }
        }
    }
}

void calcula()
{ // calcula a corrente para comparaçao do peso

    if ((millis() - currTimer) >= 200)
    {

        voltage_raw = (30 / 1023.0) * analogRead(currentSensor); // Read the voltage from sensor
        // voltage = voltage_raw - QOV + 0.012;                      // 0.000 is a value to make voltage zero when there is no current

        Serial.print("voltage_raw = ");
        Serial.println(voltage_raw);

        if (voltage_raw > 20)
        { // SOBRECARGA
            lcd.clear();
            analogWrite(LPWM_Output, 0);
            analogWrite(RPWM_Output, 0);
            Elevador.motor = 0;

            lcd.setCursor(0, 0);
            lcd.print("Sobrecarga");
            Serial.println("Sobrecarga");

            if (Elevador.state == DESCIDA_SALVADA)
            {
                lcd.setCursor(0, 1);
                lcd.print("tecle descer");
            }
            else
            {
                Elevador.state = SOBRECARGA;
                EEPROM.write(2, Elevador.state);
            }
        }

        currTimer = millis();
    }
}

void salva_descida()
{

    if (Elevador.salvandoFlag == true)
    { // SALVA VALOR

        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        Elevador.motor = 0;

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Descida Salva");
        Serial.print("Descida Salva! \n");
        lcd.setCursor(0, 1);
        lcd.print("salvar subida");
        Serial.print("Elevador.posicaoAtual = ");
        Serial.println(Elevador.posicaoAtual);

        Elevador.posicaoFinal = 0;
        Elevador.posicaoFinal = 0;
        Elevador.state = DESCIDA_SALVADA;
        EEPROM.write(0, Elevador.posicaoAtual);
        EEPROM.write(1, Elevador.posicaoFinal);
        EEPROM.write(2, Elevador.state);
        Elevador.salvandoFlag = false;
    }
    else
    { // inicia descida

        lcd.setBacklight(HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Salvando Descida");
        Serial.println("Salvando Descida");

        Elevador.salvandoFlag = true;
        Elevador.posicaoAtual = 0;
        Elevador.posicaoFinal = 0;
        Elevador.state = DESCENDO;
        EEPROM.write(0, Elevador.posicaoAtual);
        EEPROM.write(1, Elevador.posicaoFinal);
        EEPROM.write(2, Elevador.state);

        Elevador.motor = 1;
        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 170);

        delay(1000);
    }
}

void salva_subida()
{

    if (Elevador.salvandoFlag == true)
    { // SALVAR

        analogWrite(LPWM_Output, 0);
        analogWrite(RPWM_Output, 0);
        Elevador.motor = 0;

        lcd.clear();

        lcd.setCursor(0, 0);
        lcd.print("Elevador Pronto");
        Serial.print("Subida Salva \n");
        lcd.setCursor(0, 1);
        lcd.print("Tecle descer");
        Serial.println("Elevador Pronto");
        Serial.println("Tecle descer");


        Elevador.posicaoFinal = Elevador.posicaoAtual;
        Elevador.state = ELEVADOR_EM_CIMA;
        EEPROM.write(0, Elevador.posicaoAtual);
        EEPROM.write(1, Elevador.posicaoFinal);
        EEPROM.write(2, Elevador.state);
        Elevador.salvandoFlag = false;
    }
    else
    { // Iniciar subida

        lcd.setBacklight(HIGH);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Salvando Subida");
        Serial.println("Salvando Subida");

        Elevador.salvandoFlag = true;

        analogWrite(LPWM_Output, 170);
        analogWrite(RPWM_Output, 0);
        Elevador.motor = 1;

        delay(1000);
    }
}

void func_descida()
{

    Elevador.state = DESCENDO;
    // Elevador.posicaoAtual = EEPROM.read(0);
    // Elevador.posicaoFinal = EEPROM.read(1);

    lcd.setBacklight(HIGH);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Varal descendo");
    Serial.print("Varal Descendo! \n");

    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 170);
    Elevador.motor = 1;
}

void func_subida()
{

    Elevador.state = SUBINDO;
    // Elevador.posicaoAtual = EEPROM.read(0);
    // Elevador.posicaoFinal = EEPROM.read(1);

    lcd.setBacklight(HIGH);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Varal Subindo");
    Serial.print("Varal Subindo! \n");

    analogWrite(LPWM_Output, 170);
    analogWrite(RPWM_Output, 0);
    Elevador.motor = 1;
}

void func_reset()
{

    lcd.setBacklight(HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RESET");
    Serial.print("RESET \n");
    lcd.setCursor(0, 1);
    lcd.print("salvar descida");

    analogWrite(LPWM_Output, 0);
    analogWrite(RPWM_Output, 0);

    Elevador.posicaoAtual = 0;
    Elevador.posicaoFinal = 0;

    EEPROM.write(0, Elevador.posicaoAtual);
    EEPROM.write(1, Elevador.posicaoFinal);
    wdt_reset();
}
