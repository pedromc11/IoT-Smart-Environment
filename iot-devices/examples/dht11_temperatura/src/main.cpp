#include <DHT.h>
#include <Arduino.h>

#define DHTPIN 4        // Pin al que está conectado el sensor DHT11
#define DHTTYPE DHT11   // Tipo de sensor DHT que estás utilizando

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  float temperatura = dht.readTemperature(); // Lee la temperatura en grados Celsius
  float humedad = dht.readHumidity();       // Lee la humedad relativa en porcentaje

  // Verifica si la lectura de temperatura y humedad fue exitosa
  if (!isnan(temperatura) && !isnan(humedad)) {
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.print(" °C\t");
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.println(" %");
  } else {
    Serial.println("Error al leer el sensor DHT11");
  }

  delay(2000);
}
