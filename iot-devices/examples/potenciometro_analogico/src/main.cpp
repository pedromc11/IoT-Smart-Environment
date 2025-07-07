#include <DHT.h>
#include <Arduino.h>

#define POT_PIN 14      // Pin analógico al que está conectado el potenciómetro

long valor;
 
void setup() {
  //Inicializamos la comunicación serial
  Serial.begin(9600);
}
 
void loop() {
  // leemos del pin A0 valor
  valor = analogRead(POT_PIN);
 
  //Imprimimos por el monitor serie
  Serial.println("El valor es = ");
  Serial.print(valor);
  Serial.println();
  int porcentaje = map(valor,0,4095,0,100);
  Serial.print("Su porcentaje es: ");
  Serial.print(porcentaje);
  Serial.print("%");
  Serial.println();
  Serial.println();

  delay(1000);
}