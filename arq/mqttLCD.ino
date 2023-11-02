//Bibliotecas
#include "ArduinoJson.h"
#include "EspMQTTClient.h"
#include <LiquidCrystal_I2C.h>

//Declaração de variáveis
int timeTransmissao = 0;
int ReleStatus = 2;
int Rele = 19;
int btn1 = 23;
int estadoRele = 0;
int ldr = 33;
int percentLuz = 0;

// set the LCD number of columns and rows
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// variáveis para o botão:
int ultimoEstado = 1;  //o estado anterior do pino de entrada
int estadoAtual;       //a leitura atual do pino de entrada

//-------------------------------------------------//
//configurações da conexão MQTT
EspMQTTClient client(
  "FIESC_IOT",                             //nome da sua rede Wi-Fi
  "7FuhM4@Io9",                            //senha da sua rede Wi-Fi
  "mqtt.tago.io",                          // MQTT Broker server ip padrão da tago
  "Default",                               // username
  "95e40fb7-65ea-4eb4-8f2d-b2001ca9e41c",  // Código do Token DA PLATAFORMA TAGOIO
  "senaiesp002",                           // Client name that uniquely identify your device
  1883                                     // The MQTT port, default to 1883. this line can be omitted
);

//Tópico
char topicoMqtt[] = "senai/esp32/001";

/*
Esta função é chamada assim que tudo estiver conectado (Wifi e MQTT)]
AVISO: VOCÊ DEVE IMPLEMENTÁ-LO SE USAR EspMQTTClient
*/
void onConnectionEstablished() {
  Serial.println("Conectado");
  client.subscribe(topicoMqtt, onMessageReceived);
}

void onMessageReceived(const String& msg) {
  digitalWrite(ReleStatus, LOW);
  Serial.println("Mensagem recebida:");
  Serial.println(msg);

  StaticJsonBuffer<300> JSONBuffer;                  //Memory pool
  JsonObject& parsed = JSONBuffer.parseObject(msg);  //Parse message

  if (parsed.success()) {
    int ReleMqtt = parsed["rele"];
    
    if (ReleMqtt == 1) {
      ligarRele();
      Serial.println("Rele ligado por MQTT");
      lcd.setCursor(5, 1);
      lcd.print("Ligado");
    } else if (ReleMqtt == 2) {
      desligarRele();
      Serial.println("Rele desligado por MQTT");
      lcd.setCursor(5, 1);
      lcd.print("Desligado");
    }

  } else {
    Serial.println("Falha ao realizar parsing do JSON");
  }
}

void transmitirStatusRele() {
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["estadoRele"] = estadoRele;


  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  client.publish(topicoMqtt, JSONmessageBuffer);
}

/*void transmitirEstadoRele() {
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["estadoRele"] = estadoRele;


  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  client.publish(topicoMqtt, JSONmessageBuffer);
}*/

void transmitirValorLDR() {
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["ldr"] = percentLuz;

  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  client.publish(topicoMqtt, JSONmessageBuffer);
}

void setup() {
  // initialize LCD
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();

  //Ativando saída serial
  Serial.begin(9600);

  pinMode(ldr, INPUT);

  //Ativando principais portas
  pinMode(btn1, INPUT_PULLUP);
  pinMode(Rele, OUTPUT);
  digitalWrite(Rele, HIGH);

  //Informação de Placa ligada
  pinMode(ReleStatus, OUTPUT);
  digitalWrite(ReleStatus, LOW);

  Serial.printf("\nsetup() em core: %d", xPortGetCoreID());

  xTaskCreatePinnedToCore(loop2, "loop2", 8192, NULL, 1, NULL, 0);
}

void lerLdr() {
  int valorLdr = analogRead(ldr);
  percentLuz = (valorLdr * 100) / 4095;
  Serial.println("Valor Lido LDR:");
  Serial.println(percentLuz);
  transmitirValorLDR();
}

void alternarRele() {
  if (estadoRele == 0) {
    //Liga o Rele
    estadoRele = 1;
    digitalWrite(Rele, HIGH);
    Serial.println("Rele ligado por botão");
    
  } else {
    //Desliga o Rele
    estadoRele = 0;
    digitalWrite(Rele, LOW);
    Serial.println("Rele desligado por botão");
    
  }

  transmitirStatusRele();
}

void ligarRele() {
  estadoRele = 1;
  digitalWrite(Rele, LOW);
  transmitirStatusRele();
}

void desligarRele() {
  estadoRele = 0;
  digitalWrite(Rele, HIGH);
  transmitirStatusRele();
}

void exibirLCD() {

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LDR:");

  lcd.setCursor(0, 1);
  lcd.print(percentLuz);

  lcd.setCursor(2, 1);
  lcd.print("%");

  lcd.setCursor(5, 0);
  lcd.print("Rele - MQTT:");

  //delay(1000);
}


void loop() {

  /*estadoAtual = digitalRead(btn1);

  if (ultimoEstado == HIGH && estadoAtual == LOW) {
    Serial.println("Botão precionado");
    alternarRele();
  } else if (ultimoEstado == LOW && estadoAtual == HIGH) {
    Serial.println("Botão liberado");
  }

  ultimoEstado = estadoAtual;*/

  if (millis() - timeTransmissao >= 5000) {
    timeTransmissao = millis();
    lerLdr();
    transmitirStatusRele();
    exibirLCD();
    //transmitirEstadoRele();
  }
}

void loop2(void* z) {
  //Mostra no monitor em qual core o loop2() foi chamado
  Serial.printf("\nloop2() em core: %d", xPortGetCoreID());

  while (1) {
    client.loop();
    //Alternar o Rele "azul" do ESP32 conforme estado na conexão
    if (client.isWifiConnected()) {
      digitalWrite(ReleStatus, HIGH);
    } else {
      digitalWrite(ReleStatus, LOW);
    }


    delay(5);
  }
}