#include <DHT.h>
#include <Arduino.h>
#include <WiFi.h> 
#include <esp_now.h>
#include "data.h"

#define PIR_PIN 13      // Pin al que está conectado el sensor PIR
#define POT_PIN 14      // Pin analógico al que está conectado el potenciómetro
#define DHTPIN 4        // Pin al que está conectado el sensor DHT11
#define DHTTYPE DHT11   // Tipo de sensor DHT que estás utilizando

DHT dht(DHTPIN, DHTTYPE);

//Variable global de deteccion de movimiento
volatile int movement_bool = LOW;

//Variable para almacenar información sobre el nodo que se comunicará con el dispositivo
esp_now_peer_info_t peerInfo;

//Definición de estructura mensaje de envio para temperatura y humedad
struct TemperatureAndHumidity{
    float temperature;
    float humidity;
};

//Definición de estructura mensaje de envio para temperatura y humedad
struct Potenciometer{
    int potenciometer;
};

//Funciones
void movement_detected();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  printf("\r\nLast Packet Send Status:\t");
  printf(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(9600);

  // Conexión a la red WiFi
  WiFi.mode(WIFI_STA);

  //Iniciación del protocolo ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    printf("Error initializing ESP-NOW\n");
    return;
  }

  //Registrar la funcion de devolucion de llamada OnDataSent
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6); //Copia la dirección MAC del nodo destino en la estructura peerInfo 
  peerInfo.channel = 0; //Establece el canal de ESP-NOW en 0
  peerInfo.encrypt = false; //Deshabilita el cifrado de los datos enviados

  //Agrega el nodo destino como un par de comunicacion ESP-NOW
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    printf("Failed to add peer\n");
    return;
  }

  //Iniciación del sensor DHT11
  dht.begin();

  //Configuración del pin del sensor PIR como entrada
  pinMode(PIR_PIN, INPUT); 
  attachInterrupt(digitalPinToInterrupt(13), movement_detected, RISING); //detecta cambio de LOW a HIGH
}
 
void loop() {

  //Leer valores de temperatura y humedad
  float temperature = dht.readTemperature(); 
  float humidity = dht.readHumidity();

  //Leer valores del potenciómetro      
  int potValue = analogRead(POT_PIN);
  int percentaje = map(potValue,0,4095,0,100);

  //Declaracion estructura para guardar la temperatura y la humedad
  TemperatureAndHumidity temphum;
  temphum.temperature = temperature;
  temphum.humidity = humidity;

  //Envio de mensaje de la temperatura y de la humedad
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&temphum, sizeof(temphum));
  if (result == ESP_OK){
    printf("Sent (temphum) with success: %s and %d\n", temphum.temperature, temphum.humidity);
  } else {
    printf("Error sending (temphum)\n");
  }

  //Declaracion estructura para guardar el valor del potenciometro
  Potenciometer pot;
  pot.potenciometer = potValue;

  //Envio de mensaje de la temperatura y de la humedad
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&pot, sizeof(pot));
  if (result == ESP_OK){
    printf("Sent (pot) with success: %s\n", pot.potenciometer);
  } else {
    printf("Error sending (pot)\n");
  }

  delay(1000);
}

//Funcion que sirve para cambiar la variable movement_bool cuando ocurre un movimiento
void movement_detected(){
  movement_bool = HIGH;
}
