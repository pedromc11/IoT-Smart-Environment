#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <time.h>
#include <sys/time.h>
#include <PubSubClient.h>
#include "data.h"

#define RTC_SYNC_INTERVAL 3600000               // Intervalo de sincronización del RTC en milisegundos (1 hora)

// RCN ¿localhost? Debería ser la IP del broker MQTT
#define MQTT_BROKER "localhost"                 // Dirección del broker MQTT
#define MQTT_PORT 5001                          // Puerto del broker MQTT
#define MQTT_USER "student"                     // Usuario para autenticación en el broker MQTT
#define MQTT_PASSWORD "1234"                    // Contraseña para autenticación en el broker MQTT
#define ID_RED_IOT_PRIVADA "gateway.node.esp32" // Identificador de la red IoT privada

// RCN RTC_DATA_ATTR es un atributo utilizado para declarar variables que deben ser almacenadas en la memoria RTC (Real-Time Clock) de un microcontrolador. La memoria RTC se conserva durante los reinicios y las entradas/salidas de modo de baja energía (deep sleep), lo que permite que las variables mantengan su valor a través de estos eventos. No obstante, si apagas la placa y vuelves a encender, el dato no se mantiene.
RTC_DATA_ATTR int rebootCount = 0; // Contador de reinicio
unsigned long lastWakeTime;        // Contador del tiempo activo

WiFiClient wifiClient;               // Creación de un cliente WiFi
PubSubClient mqttClient(wifiClient); // Creación del cliente MQTT utilizando el cliente WiFi

// RCN ¿Para qué necesitas un semáforo?
SemaphoreHandle_t rtcSemaphore; // Semáforo para la sincronización del RTC

void internal_RTC_updater(void *parameter);                                               // Declaración de la función para actualizar el RTC internamente
void handle_RTC_sync_request(const uint8_t *mac_addr, const uint8_t *data, int data_len); // Declaración de la función para manejar las solicitudes de sincronización RTC
void setupWiFi();                                                                         // Declaración de la función para configurar la conexión WiFi
void connectToMQTTBroker();                                                               // Declaración de la función para conectar con el broker MQTT
void publishToMQTT(const char *topic, const char *payload);                               // Declaración de la función para publicar mensajes en MQTT
void configTimeAndSync();                                                                 // Declaración de la función para configurar y sincronizar el tiempo
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);              // Declaración de la función para recibir datos por ESP-NOW

void setup()
{
  Serial.begin(9600);

  WiFi.mode(WIFI_STA); // Configuración del modo WiFi en estación (cliente)
  setupWiFi();         // Llamada a la función de configuración de WiFi

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error al inicializar ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv); // Registro del callback para recibir datos por ESP-NOW

  rtcSemaphore = xSemaphoreCreateMutex(); // Creación del semáforo para la sincronización del RTC

  xTaskCreatePinnedToCore(internal_RTC_updater, "RTC Updater", 4096, NULL, 1, NULL, 1);
}

void loop()
{
  // RCN esto no es necesario hacerlo dentro de loop()
  if (!mqttClient.connected()) // Verificación de conexión con el broker MQTT
  {
    connectToMQTTBroker(); // Conexión al broker MQTT si no está conectado
  }
  mqttClient.loop(); // Función de loop para mantener la comunicación con el broker MQTT
  delay(100);
}

void internal_RTC_updater(void *parameter)
{
  for (;;)
  {
    // RCN No tiene sentido un mutex, es la única tarea que lo emplea. En cualquier caso, es innecesario
    if (xSemaphoreTake(rtcSemaphore, portMAX_DELAY))
    {
      configTimeAndSync();          // Llamada a la función para configurar y sincronizar el tiempo
      xSemaphoreGive(rtcSemaphore); // Liberación del semáforo después de la sincronización
    }
    vTaskDelay(pdMS_TO_TICKS(RTC_SYNC_INTERVAL));
  }
}

// RCN ¿para que necesitas el mac_addr y data_len? No lo usas...
// RCN no estas contestando por espnow en ningun sitio a la placa que te solicita la hora
void handle_RTC_sync_request(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  TimeRequest request;
  memcpy(&request, data, sizeof(request)); // Copia de los datos recibidos en la estructura

  if (strcmp(request.request, "TIME") == 0)
  {
    configTimeAndSync();
  }
}

void setupWiFi()
{
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conectado a WiFi");
}

void connectToMQTTBroker()
{
  while (!mqttClient.connected()) // Mientras no esté conectado al broker
  {
    Serial.print("Conectando al broker MQTT...");
    if (mqttClient.connect("ESP32Client", MQTT_USER, MQTT_PASSWORD)) // Intento de conexión al broker MQTT
    {
      Serial.println("Conectado");
    }
    else
    {
      Serial.print("fallo, estado ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void publishToMQTT(const char *topic, const char *payload)
{
  // RCN no es coherente con los canales que usas en el dummy. Deberías publicar en función del tipo de mensaje
  mqttClient.publish(topic, payload);
}

void configTimeAndSync()
{
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Sincronizando con NTP...");
  while (!time(nullptr))
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("sincronizado!");

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Fallo al obtener el tiempo");
    return;
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  // RCN lo correcto sería identificar el mensaje mediante una estrategia que no dependa del tamaño del payload. Lo común suele ser reservar algún byte del payload para indicar el tipo de mensaje.
  if (data_len == sizeof(TimeRequest))
  {
    handle_RTC_sync_request(mac_addr, data, data_len);
  }
  else if (memcmp(mac_addr, sensorMAC, sizeof(sensorMAC)) == 0) // RCN ¿Solo contestas a esa mac? ¿Y si cambias la placa? No tiene sentido...
  {
    char message[100];
    snprintf(message, sizeof(message), "Se ha mandado al broker los datos: %s", (char *)data);
    Serial.println(message); // Imprimir el mensaje antes de publicarlo
    publishToMQTT("evento_salida", message); // RCN ¿Siempre lo publicas al mismo canal? ¿no deberías publicar en función del tipo de mensaje? De nuevo, sin mucho sentido...
  }
}