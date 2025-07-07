#include <DHT.h>
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <time.h>
#include "data.h"

#define DHTPIN 4      // Pin al que está conectado el sensor DHT11
#define DHTTYPE DHT11 // Tipo de sensor DHT que estás utilizando
#define PIR_PIN 13    // Pin al que está conectado el sensor PIR
#define POT_PIN 14    // Pin analógico al que está conectado el potenciómetro

DHT dht(DHTPIN, DHTTYPE);

volatile int movimiento_bool = LOW; // Variable global de deteccion de movimiento

float temperatura, humedad;      // Variable para almacenar las lecturas de humedad y temperatura
int porcentaje;                  // Variable para almacenar el porcentaje del valor del potenciómetro
float lastTemperatura = -1000.0; // Variable para almacenar la temperatura anterior
float lastHumedad = -1000.0;     // Variable para almacenar la humedad anterior
int lastPorcentaje = -1000;      // Variable para alamcenar el porcentaje anterior

const float deltaTemperatura = 0.5; // Umbral delta para la temperatura
const float deltaHumedad = 2.0;     // Umbral delta para la humedad
const int deltaPorcentaje = 5;      // Umbral delta para el porcentaje

DataReading dataBuffer[10]; // Buffer para almacenar las lecturas en batch
int dataIndex = 0;          // Indice del buffer

// RCN esta variable no se conserva entre reinicios, solo cuando el microcontrolador entra en modo reposo profundo
RTC_DATA_ATTR int rebootCount = 0; // Contador de reinicio
unsigned long lastWakeTime;        // Contador del tiempo activo

SemaphoreHandle_t rtcSemaphore; // Semaforo para la sincronizacion del RTC

void movimiento_detectado();                                                 // Metodo para cambiar la variable movimiento_bool a HIGH
void verificarYenviarDatos(void *parameter);                                 // Metodo para enviar los datos al gateway.node.esp32 aplicando el algoritmo send on delta
void enviarDatosBatch();                                                     // Metodo para enviar datos al gateway mediante el protocolo ESPNOW en batería o en batch
time_t obtenerTiempoUTC();                                                   // Metodo para obtener el tiempo en formato UTC
void configTimeAndSync();                                                    // Metodo que pide la hora al nodo gateway usando ESPNOW
void temperature_humidity_updater(void *parameter);                          // Tarea FreeRRTOS encargada de realizar las lecturas de temperatura y humedad y enviarlas al gateway.node.esp32 mediante ESPNOW
void analog_potentiometer_updater(void *parameter);                          // Tarea FreeRTOS encargada de realizar las lecturas del potenciómetro y enviarlas al gateway.node.esp32 mediante ESPNOW
void internal_RTC_updater(void *parameter);                                  // Tarea FreeRTOS encargada de gestionar la deriva del RTC interno del microcontrolador
void presence_updater(void *parameter);                                      // Tarea FreeRTOS encargada de notificar cuando el PIR detecta presencia al nodo gateway.node.esp32 mediante el protocolo ESPNOW
void board_status_updater(void *parameter);                                  // Tarea FreeRTOS  encargada de enviar información cada minuto sobre el estado del nodo al nodo gateway.node.esp32 mediante el protocolo ESPNOW
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len); // Callback para recibir datos de gateway.node.esp32

void setup()
{
  Serial.begin(9600);

  dht.begin();
  pinMode(PIR_PIN, INPUT); // Configurar el pin del sensor PIR como entrada

  attachInterrupt(digitalPinToInterrupt(13), movimiento_detectado, RISING); // Detecta cambio de LOW a HIGH en el sensor PIR

  // RCN esta nodo no debe conectarse a la red Wifi. SOLO se comunica por ESPNOW
  WiFi.mode(WIFI_STA); // Inicializacion del WiFi

  if (esp_now_init() != ESP_OK) // Inicialización de ESPNOW
  {
    Serial.println("Error al inicializar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv); // Registrar el callback para recibir datos

  rebootCount++;           // Incrementar el contador de reinicio
  lastWakeTime = millis(); // Actualizar la última vez que se desperto

  rtcSemaphore = xSemaphoreCreateMutex(); // Creacion del semaforo

  // RCN Sería mejor no indicar el core en el que se ejecuta la tarea. Deja que FreeRTOS lo decida.
  xTaskCreatePinnedToCore(temperature_humidity_updater, "Leer Temperatura y Humedad", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(analog_potentiometer_updater, "Leer Potenciómetro", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(verificarYenviarDatos, "Verificar y Enviar Datos Batch", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(internal_RTC_updater, "Actualizar RTC", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(presence_updater, "Notificar Presencia", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(board_status_updater, "Actualizar Estado Nodo", 2048, NULL, 1, NULL, 1);
}

void loop()
{
  // El loop esta vacio debido a que todas las tareas se manejan en FreeRTOS
}

void temperature_humidity_updater(void *parameter)
{
  for (;;)
  {
    temperatura = dht.readTemperature(); // Leer temperatura en grados Celsius
    humedad = dht.readHumidity();        // Leer humedad relativa en porcentaje
    vTaskDelay(pdMS_TO_TICKS(20000));    // Esperar 20 segundos antes de realizar otra lectura
  }
}

void analog_potentiometer_updater(void *parameter)
{
  for (;;)
  {
    int valor = analogRead(POT_PIN);          // Leer valor del potenciómetro
    porcentaje = map(valor, 0, 4095, 0, 100); // Convertirlo a porcentaje
    vTaskDelay(pdMS_TO_TICKS(20000));         // Esperar 20 segundos antes de realizar otra lectura
  }
}

// RCN solución parcialmente correcta. Usas interrupciones, pero compruebas cada 1 segundo, por lo que la marca de tiempo puede tener un error de hasta 1 segundo.
void presence_updater(void *parameter)
{
  for (;;)
  {
    if (movimiento_bool == HIGH)
    {
      // Enviar notificación de presencia usando ESPNOW
      PresenceNotification notificacion;
      notificacion.presencia = true;
      notificacion.timestamp = obtenerTiempoUTC();

      esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *)&notificacion, sizeof(notificacion));

      movimiento_bool = LOW; // Resetear el indicador de movimiento
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Verificar cada segundo
  }
}

void movimiento_detectado()
{
  movimiento_bool = HIGH;
}

void board_status_updater(void *parameter)
{
  for (;;)
  {
    unsigned long currentTime = millis();                       // Obtencion del tiempo actual
    unsigned long uptime = (currentTime - lastWakeTime) / 1000; // Conversion a segundos

    // RCN ¿Y la marca de tiempo?
    NodeStatus status;
    status.rebootCount = rebootCount;
    status.uptime = uptime;

    esp_now_send(gatewayAddress, (uint8_t *)&status, sizeof(status)); // Enviar estado del nodo usando ESPNOW

    vTaskDelay(pdMS_TO_TICKS(60000)); // Enviar cada minuto
  }
}

void internal_RTC_updater(void *parameter)
{
  for (;;)
  {
    // RCN ¿Para qué un semáforo? 
    if (xSemaphoreTake(rtcSemaphore, portMAX_DELAY))
    {
      configTimeAndSync();
      xSemaphoreGive(rtcSemaphore);
    }
    vTaskDelay(pdMS_TO_TICKS(60000)); // Esperar 1 minuto antes de la próxima actualización
  }
}

void configTimeAndSync()
{
  TimeRequest request;

  esp_err_t result = esp_now_send(gatewayAddress, (uint8_t *)&request, sizeof(request)); // Enviar solicitud de tiempo
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  if (data_len == sizeof(time_t)) // Verificar si los datos recibidos corresponden a una respuesta de tiempo
  {
    time_t receivedTime;
    memcpy(&receivedTime, data, sizeof(time_t));

    // Establecer el tiempo local con la respuesta recibida
    struct timeval tv;
    tv.tv_sec = receivedTime;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    Serial.println("Tiempo sincronizado con éxito");
  }
}

// RCN Segun el enunciado, cada tipo de datos debe enviarse desde diferentes tareas.
void verificarYenviarDatos(void *parameter)
{
  for (;;)
  {
    bool sendTemperatura = abs(temperatura - lastTemperatura) >= deltaTemperatura; // Variable que compara si la diferencia absoluta entre temperatura y lastTemperatura es mayor o igual que deltaTemperatura
    bool sendHumedad = abs(humedad - lastHumedad) >= deltaHumedad;                 // Variable que compara si la diferencia absoluta entre humedad y lastHumedad es mayor o igual que deltaHumedad
    bool sendPorcentaje = abs(porcentaje - lastPorcentaje) >= deltaPorcentaje;     // Variable que compara si la diferencia absoluta entre porcentaje y lastPorcentaje es mayor o igual que deltaPorcentaje

    if (sendTemperatura || sendHumedad || sendPorcentaje) // Si ha habido alguna variacion
    {
      DataReading reading;
      reading.temperatura = temperatura;      // Almacenar la lectura actual de la temperatura en el buffer
      reading.humedad = humedad;              // Almacenar la lectura actual de la humedad en el buffer
      reading.porcentaje = porcentaje;        // Almacenar la lectura actual del porcentaje en el buffer
      reading.timestamp = obtenerTiempoUTC(); // Almacenar el timestamp en el buffer

      dataBuffer[dataIndex++] = reading; // Guardar la lectura actual en el buffer

      // Actualizar las lecturas anteriores solo si se enviaron
      if (sendTemperatura)
        lastTemperatura = temperatura;
      if (sendHumedad)
        lastHumedad = humedad;
      if (sendPorcentaje)
        lastPorcentaje = porcentaje;

      if (dataIndex >= 10) // Si el buffer esta lleno
      {
        enviarDatosBatch(); // Enviar datos al batch
        dataIndex = 0;      // Reiniciar el indice del buffer
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Verificar cada segundo
  }
}

time_t obtenerTiempoUTC()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Ocurrio un error al obtener el tiempo");
    return 0;
  }
  time(&now);
  return now;
}

void enviarDatosBatch()
{
  esp_now_send(gatewayAddress, (uint8_t *)dataBuffer, sizeof(dataBuffer));
}
