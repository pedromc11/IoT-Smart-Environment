#include <DHT.h>
#include <Arduino.h>

#define PIR_PIN 13      // Pin al que está conectado el sensor PIR

// Variable global de deteccion de movimiento
volatile int movimiento_bool = LOW;
void movimiento_detectado();
void setup() {
  Serial.begin(9600);
  pinMode(PIR_PIN, INPUT); // Configurar el pin del sensor PIR como entrada
  attachInterrupt(digitalPinToInterrupt(13), movimiento_detectado, RISING); //detecta cambio de LOW a HIGH
}

void loop() {

  if (movimiento_bool == HIGH) {
    Serial.println("¡Movimiento detectado!");
    movimiento_bool = LOW;
  } else {
    Serial.println("Sin movimiento");
  }

  delay(1000); // Esperar un segundo antes de realizar otra lectura
}

void movimiento_detectado(){
  movimiento_bool = HIGH;
}
