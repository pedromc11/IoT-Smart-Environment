uint8_t sensorMAC[] = {0xA0, 0xDD, 0x6C, 0x10, 0x81, 0x40};

typedef struct // Estructura para el almacenamiento de una lectura con su timestamp
{
  float temperatura;
  float humedad;
  int porcentaje;
  time_t timestamp;
} DataReading;

typedef struct // Estructura para el almacenamiento de la notificación de la presencia
{
  bool presencia;
  time_t timestamp;
} PresenceNotification;

typedef struct // Estructura para solicitar el tiempo al nodo gateway
{
  char request[5] = "TIME";
} TimeRequest;

typedef struct // Estructura para almacenar el estado del nodo
{
  int rebootCount;
  unsigned long uptime;
} NodeStatus;
