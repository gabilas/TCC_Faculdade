//Configurações para conexão com o Blynk
#define BLYNK_TEMPLATE_ID "TMPLqnALNXFA"
#define BLYNK_DEVICE_NAME "Medidor TCC ESP32"
#define BLYNK_AUTH_TOKEN  "xxxxxx-xxxxxxxx-xxxxxxxxx" //Token de autorização para comunicar o blynk com meu projeto TCC. Por segurança esse código não será disponibilizado aqui no GIT.
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp32.h>

//Bibliotecas para Conexão Wi-Fi
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiManager.h>

//Configurações da tela do ESP32 TTGO
#include <TFT_eSPI.h>
#define TFT_GREY 0x5AEB
#define lightblue 0x01E9
#define darkred 0xA041
#define blue 0x5D9B

//Configurações para gravação na memoria Flash do ESP32
#include <EEPROM.h>
#define EEPROM_SIZE 6

//Definição de GPIOs
#define reset_WiFi 35
#define reset_dados 0

#define sensor_corrente 39
#define sensor_tensao 32
#define oz 33

#define LED 13

TFT_eSPI tft = TFT_eSPI();
WiFiManager wm;

//Variáveis
String ID = "";
String senha = "";

int segundos = 0, minutos = 0, horas = 0, dias = 0, adcC = 0, adcT = 0;
float Vp_IAC = 0, corrente_pico = 0, corrente_rms = 0, Vp_VAC = 0, Vp_VAC_R = 0, tensao_rms = 0, potencia = 0, energia = 0, e_acumulado = 0, corrente_a = 0, VP_VAC_a = 0;

int address = 0;

String SECONDS = "";
String MINUTES = "";
String HOURS = "";

//Constantes 
char auth[] = BLYNK_AUTH_TOKEN;

const float offset_corrente = 1290; //Valor de offset médio
const float coefA_IAC = 0.8885; //Coeficiente para calibrar valor de corrente
const float coefB_IAC = 0.0051; //Coeficiente para calibrar valor de corrente

const float offset_tensao = 22; //Valor de offset médio
const float coefA_VAC = 1.0139; //Coeficiente para calibrar valor de tensão
const float coefB_VAC = 0.0994; //Coeficiente para calibrar valor de tensão

BlynkTimer timer; //Iniciar Timer do BLYNK

void dados() //Verifica valores previamente gravados na memória do ESP
{
  EEPROM.begin(EEPROM_SIZE); //Acessar memoria do ESP32
  address = 0;
 
  int r_sec;
  r_sec = EEPROM.read(address); //Ler valor gravado no endereço
  Serial.print("Segundos = ");
  Serial.println(r_sec);
  segundos = r_sec;
  address += sizeof(r_sec); //Atualizar valor do endereço

  int r_min;
  r_min = EEPROM.read(address);
  Serial.print("Minutos = ");
  Serial.println(r_min);
  minutos = r_min;
  address += sizeof(r_min);

  int r_hrs;
  r_hrs = EEPROM.read(address);
  Serial.print("Horas = ");
  Serial.println(r_hrs);
  horas = r_hrs;
  address += sizeof(r_hrs);

  int r_day;
  r_day = EEPROM.read(address);
  Serial.print("Dias = ");
  Serial.println(r_day);
  dias = r_day;
  address += sizeof(r_day);

  float r_acu;
  r_acu = EEPROM.read(address);
  Serial.print("Energia acumulada = ");
  Serial.println(r_acu);
  e_acumulado = r_acu;
  address += sizeof(r_acu);

  EEPROM.end();
  }

void conectar() //Conectar o ESP com uma rede WiFi
{

  tft.init(); //Iniciar Display do ESP32
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  tft.print("Conecte-se ao ESP para configurar a rede Wi-Fi!!!");

  bool res;
  res = wm.autoConnect("Gabilas_EPSTTGO","soeuseiasenha"); //Configurar AP do ESP32 para realizar a conexão

  tft.print("Conectando ao WiFi: ");
  tft.println(WiFi.SSID());
  
  while (WiFi.status() != WL_CONNECTED) {
    while (res){
    delay(300);
    tft.print(".");
    } 
  }
  if (!res) {tft.println("");
        tft.print("Não consegiu conectar... tente novamente");
        delay(1000);
        ESP.restart();}

  tft.println("");
  tft.println("Conectado ao WiFi: ");
  tft.println(WiFi.SSID());
  tft.println("Endereço IP: ");
  tft.println(WiFi.localIP());
  delay(3000);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);

  if(digitalRead(reset_WiFi) == LOW){
    wm.resetSettings();
    ESP.restart();
    }
    
  }

void myTimerEvent() //Contador de tempo. Conta de segundos em segundos
{
  int sec = millis() / 1000;
  
  segundos = segundos+1;
  
  if (segundos >= 60){
    minutos = minutos +1;
    segundos = 0;
    
    if (minutos >= 60){
      horas = horas +1;
      minutos = 0;
      
      if (horas >= 24){
        dias = dias +1;
        horas = 0;
       }
     }
   }

   if (segundos < 10){SECONDS = "0" + String(segundos);}
   if (segundos >= 10){SECONDS = String(segundos);}
   if (minutos < 10){MINUTES = "0" + String(minutos);}
   if (minutos >= 10){MINUTES = String(minutos);}
   if (horas < 10){HOURS = "0" + String(horas);}
   if (horas >= 10){HOURS = String(horas);}

   //Enviar dados do tempo para a plataforma Blynk
   Blynk.virtualWrite(V0, dias);
   Blynk.virtualWrite(V1, horas);
   Blynk.virtualWrite(V2, minutos);
   Blynk.virtualWrite(V3, segundos);

   //Gravar dados do tempo na memória do ESP32
   EEPROM.begin(EEPROM_SIZE);
   address = 0;
   EEPROM.write(address, segundos);
   address += sizeof(segundos);
   EEPROM.write(address, minutos);
   address += sizeof(minutos);
   EEPROM.write(address, horas);
   address += sizeof(horas);
   EEPROM.write(address, dias);
   address += sizeof(dias);
   EEPROM.commit();
   EEPROM.end();
}

void Medidor() //Medidor de corrente e tensão elétrica
{
  pinMode(sensor_corrente, INPUT);
  pinMode(sensor_tensao, INPUT);

  VP_VAC_a = 0, corrente_a = 0;

  for (int i = 0; i < 60; i++){ // Realizar 60 leituras
    //Período mínimo de captura de 16,7ms para garantir que ao menos um ciclo seja capturado, para um periodo de 1s com frequência de 60Hz
    const int ciclo = (int32_t)(1000/60);
    
    int maior_valorC = 0, valor_lidoC = 0, maior_valorT = 0, valor_lidoT = 0;
  
    unsigned long inicio = millis(); //Inicio do ciclo de leitura
    do
    {    
      valor_lidoC = analogRead(sensor_corrente);
      valor_lidoT = analogRead(sensor_tensao) - analogRead(oz); //Comparação entre os valores coletados nos terminais do resistor.

      //Print Serial dos valores coletado
      Serial.print(valor_lidoT);
      Serial.println("V");
      Serial.print(valor_lidoC);
      Serial.println("A");
  
      if(valor_lidoC > maior_valorC){
        maior_valorC = valor_lidoC; //Maior valor lido dentro do ciclo
      }
      if(valor_lidoT > maior_valorT){
        maior_valorT = valor_lidoT; //Maior valor lido dentro do ciclo
      }
      
    } while ((millis() - inicio) < ciclo); //Realizar leitura enquanto tempo decorrido é inferior a um ciclo de 60Hz

    adcC = maior_valorC - offset_corrente; //Valor ADC de pico
    Vp_IAC = adcC* (3.3/4095); //Multiplicar o valor ADC pela resolução mínima de leitura para o ESp32 (3.3/4095) para encontrar o valor em V
    corrente_pico = ((Vp_IAC/0.066)*coefA_IAC) - coefB_IAC; //Converter o valor de tensão de pico para corrente de pico de acordo com a sensibilidade do sensor (66 mV/A), e então aplicar a correção do valor
    corrente_a = corrente_a + corrente_pico; //Acumula as leituras

    adcT = maior_valorT - offset_tensao; //Valor ADC de pico
    Vp_VAC = adcT*(3.3/4095); //Multiplicar o valor ADC pela resolução mínima de leitura para o ESp32 (3.3/4095) para encontrar o valor em V
    Vp_VAC_R = ((Vp_VAC*(41440/220))*coefA_VAC) - coefB_VAC; //Aplicar proporcionalidade do valor coletado em relação a fonte, e então aplicar a correção do valor
    VP_VAC_a = VP_VAC_a + Vp_VAC_R; //Acumula as leituras
  }

  corrente_rms = (corrente_a/60)/sqrt(2); //Realiza a média dos valores coletados e então encontra o valor eficaz
  tensao_rms = (VP_VAC_a/60)/sqrt(2); //Realiza a média dos valores coletados e então encontra o valor eficaz

  potencia = (tensao_rms * corrente_rms)/1000; //Potência em kW

  energia = potencia/3600; //Energia consumida num periodo de 1s
  e_acumulado = e_acumulado + energia; //Acumula o valor da energia consumida

  //Envia os valores coletados ao Blynk
  Blynk.virtualWrite(V5, corrente_rms);
  Blynk.virtualWrite(V6, tensao_rms);
  Blynk.virtualWrite(V7, potencia);
  Blynk.virtualWrite(V8, e_acumulado);

  //Grava os valores coletados na memória do ESP
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(address, e_acumulado);
  EEPROM.commit();
  EEPROM.end();
}

//Controle de carga pela plataforma do Blynk
 BLYNK_WRITE(V9)
{
  int value = param.asInt();
  digitalWrite(LED, value);
  Blynk.virtualWrite(V10, value);
}

void setup()
{
  Serial.begin(115200); //Inicia o Serial

  //Define entradas e saídas
  pinMode(reset_WiFi, INPUT_PULLUP);
  pinMode(reset_dados, INPUT_PULLUP);
  pinMode(sensor_corrente, INPUT);
  pinMode(sensor_tensao, INPUT);
  pinMode(LED, OUTPUT);

  dados(); //Executa a função dados

  conectar(); //Executa a função conecetar
  
  ID = WiFi.SSID(); //Coleta ID da rede configurada
  senha = WiFi.psk(); //Coleta senha da rede configurada

  //Conversão dos dados de configuração da rede para poder conectar-se com a plataforma Blynk
  int ID_len = ID.length() + 1;
  int senha_len = senha.length() + 1; 

  char ssid[ID_len];
  char pass[senha_len];
  
  ID.toCharArray(ssid, ID_len);
  senha.toCharArray(pass, senha_len);
  
  Blynk.begin(auth, ssid, pass);

  Blynk.virtualWrite(V4, ssid);

  timer.setInterval(1000L, myTimerEvent); //Executar a função do Timer
  timer.setInterval(1000L, Medidor); //Executar a função do medidor

  //Print inicial no display do ESP
  tft.setCursor(2, 232, 1);
  tft.println("Wi-Fi: " + String(WiFi.SSID()));

  tft.setTextColor(TFT_WHITE,lightblue);
  tft.setCursor(80, 160, 2);
  tft.println("Energia:");
  tft.setCursor(80, 100, 2);
  tft.println("Tensao:");
  tft.setCursor(4, 100, 2);
  tft.println("Corrente:");
  tft.setCursor(4, 160, 2);
  tft.println("Potencia: ");
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextFont(3);
  tft.setCursor(1, 10, 2);
  tft.println("Esta ligado a: ");

  tft.fillRect(68,100,1,130,TFT_GREY);
}
 
void loop()
{
  if(digitalRead(reset_WiFi) == LOW){ // Reset da rede WiFi
    wm.resetSettings();
    ESP.restart();
    }
    
  if(digitalRead(reset_dados) == LOW){ //Reset dos dados gravados
    int zero = 0;
    for (int i = 0 ; i < EEPROM_SIZE ; i++) {
      EEPROM.begin(EEPROM_SIZE);
      EEPROM.write(i, zero);
      EEPROM.commit();
      EEPROM.end();
      }  
  ESP.restart();
  }
 
  Blynk.run();
  timer.run();
  
  //Atualização dos dados no Display do ESP32
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(95, 10, 2);
  tft.println(String(dias)+" dias");

  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(40,50,2);
  tft.println(HOURS+":"+MINUTES+":"+SECONDS);

  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setCursor(80, 195, 2);
  tft.println(energia, 2);
  tft.setCursor(80, 130, 2);
  tft.println(tensao_rms, 2);
  tft.setCursor(4, 130, 2);
  tft.println(corrente_rms, 2);
  tft.setCursor(4, 195, 2);
  tft.println(potencia, 2);
}
