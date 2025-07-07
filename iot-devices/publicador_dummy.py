import paho.mqtt.client as mqtt
import threading
import time
import random
import json
import signal
import sys

# Configuración del broker MQTT
MQTT_BROKER = "localhost"               # Dirección del broker MQTT
MQTT_PORT = 5001                        # Puerto del broker MQTT
MQTT_USER = "student"                   # Usuario para autenticación en el broker MQTT
MQTT_PASSWORD = "1234"                  # Contraseña para autenticación en el broker MQTT
ID_RED_IOT_PRIVADA = "publicador_dummy" # Identificador de la red IoT privada

def generate_data(min_value, max_value): # Función que retorna un valor aleatorio dado un minimo y un maximo
    return random.uniform(min_value, max_value)

def publish_data(client, node_id, data_type, value):
    topic = f"/{ID_RED_IOT_PRIVADA}/{data_type}/{node_id}"           # Construcción del tema MQTT con el identificador de red, tipo de datos y ID del nodo
    payload = json.dumps({"valor": value, "timestamp": time.time()}) # Payload en formato JSON con el valor y timestamp
    client.publish(topic, payload)                                   # Publicación del mensaje en el tema MQTT usando el cliente MQTT proporcionado
    print(f"Publicado al tema {topic}: {payload}")

class SensorNode:
    def __init__(self, node_id):
        self.node_id = node_id                                # Asignación del ID del nodo que se pasa como argumento al inicializar la instancia
        self.client = mqtt.Client()                           # Creacion de instancia del cliente MQTT
        self.client.username_pw_set(MQTT_USER, MQTT_PASSWORD) # Configuración del nombre de usuario y contraseña para la conexión MQTT
        self.client.connect(MQTT_BROKER, MQTT_PORT, 60)       # Conexión del cliente MQTT al broker especificado con el puerto y con el tiempo de espera
        self.running = True                                   # Flag para controlar la ejecución de los hilos
        self.sigint_received = False                          # Flag para indicar si se ha recibido SIGINT

    def start(self):
        self.threads = [                                                # Creación de la lista de hilos independientes
            threading.Thread(target=self.temperature_humidity_updater), # Hilo para actualizar temperatura y humedad
            threading.Thread(target=self.presence_updater),             # Hilo para actualizar presencia
            threading.Thread(target=self.potentiometer_updater)         # Hilo para actualizar el potenciómetro
        ]
        for thread in self.threads:
            thread.start() # Inicialización de cada hilo en la lista

    def stop(self):
        self.running = False            # Cambia el flag para detener los hilos
        for thread in self.threads:
            if thread.is_alive():
                thread.join(timeout=1)  # Espera máximo 1 segundo para que los hilos terminen

    def temperature_humidity_updater(self):
        while self.running and not self.sigint_received:
            temperature = generate_data(-5, 45)                                 # Genera datos de temperatura aleatorios
            humidity = generate_data(0, 100)                                    # Genera datos de humedad aleatorios
            publish_data(self.client, self.node_id, "temperature", temperature) # Publica los datos de temperatura
            publish_data(self.client, self.node_id, "humidity", humidity)       # Publica los datos de humedad
            time.sleep(5)                                                       # Espera 5 segundos antes de la próxima actualización

    def presence_updater(self):
        while self.running and not self.sigint_received:
            presence_interval = random.randint(5, 15)              # Intervalo aleatorio para la presencia
            time.sleep(presence_interval)                          # Espera según el intervalo de presencia
            publish_data(self.client, self.node_id, "presence", 1) # Publica datos de presencia (valor 1)

    def potentiometer_updater(self):
        while self.running and not self.sigint_received:
            pot_interval = random.randint(5, 15)                                          # Intervalo aleatorio para el potenciómetro
            time.sleep(pot_interval)                                                      # Espera según el intervalo del potenciómetro
            potentiometer_value = generate_data(0, 100)                                   # Genera un valor aleatorio para el potenciómetro
            publish_data(self.client, self.node_id, "potentiometer", potentiometer_value) # Publica el valor del potenciómetro

def signal_handler(sig, frame):
    print("Exiting...")
    for node in nodes:
        node.sigint_received = True     # Indicar a los nodos que se ha recibido la señal SIGINT
        node.stop()                     # Detener los nodos forzadamente
    sys.exit(0)

if __name__ == "__main__":

    signal.signal(signal.SIGINT, signal_handler) # Configura el manejador de señal SIGINT

    if len(sys.argv) != 2:
        print("Error: python dummy_publisher.py <number_of_nodes>")
        sys.exit(1)

    number_of_nodes = int(sys.argv[1])                                # Número de nodos a crear
    nodes = [SensorNode(f"node_{i}") for i in range(number_of_nodes)] # Crea una lista de nodos SensorNode

    for node in nodes:
        node.start() # Inicializacion de cada nodo

    while True:
        time.sleep(1) # Esperar indefinidamente hasta recibir SIGINT
