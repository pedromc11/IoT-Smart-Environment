uint8_t gatewayAddress[] = {0x10, 0x06, 0x1C, 0xBA, 0x1A, 0x00};

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
