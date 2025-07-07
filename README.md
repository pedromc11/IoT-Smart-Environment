# IoT Smart Environment Monitoring System 🏡📊

This repository contains the essential components for an IoT system designed to monitor environmental conditions and presence. It leverages ESP32 microcontrollers for sensing and gateway functionalities, and a Raspberry Pi as a central server running an MQTT broker. A Python-based dummy publisher is also included for synthetic data generation and testing.

---

## Project Overview

This project implements a local IoT network consisting of two main types of nodes and a central broker:
1.  **`sensor.node.esp32`**: Reads data from various sensors, processes it, and sends it to a local IoT gateway.
2.  **`gateway.node.esp32`**: Receives data from sensor nodes and publishes it to an MQTT broker.
3.  **MQTT Broker**: An **Eclipse Mosquitto** instance managed via **Docker** on a **Raspberry Pi**.

A **Dummy Publisher** is also provided to simulate sensor data for testing and development purposes.

---

## Components

### sensor.node.esp32

This node is based on an **ESP32 DevKit V1** and integrates the following sensors:
* **DHT11 Module**: For humidity and temperature readings.
* **PIR HC-SR501 Module**: For motion detection.
* **Analog Potentiometer**: For analog value readings.

**Functionality:**
* Reads temperature, humidity, and potentiometer values every **20 seconds**.
* Implements a **"send on delta"** algorithm: data is only sent if there's a significant change (`+/-Δ`) from the previous reading.
* Sends data to `gateway.node.esp32` using **ESPNOW** in **batch mode** (multiple readings in one message).
* Each reading in a batch includes a **UTC timestamp** indicating when the data was taken.
* Leverages **FreeRTOS tasks** for concurrency and optimal ESP32 core utilization.

**Mandatory FreeRTOS Tasks:**
* **`internal_RTC_updater`**: Manages the internal RTC drift. Queries `gateway.node.esp32` for the current time via ESPNOW every minute and updates its RTC. Includes mechanisms to handle communication delays/processing issues.
* **`temperature_humidity_updater`**: Reads temperature and humidity data and sends it to `gateway.node.esp32` via ESPNOW when the "send on delta" condition is met.
* **`analog_potentiometer_updater`**: Reads potentiometer values and sends them to `gateway.node.esp32` via ESPNOW when the "send on delta" condition is met.
* **`presence_updater`**: Notifies `gateway.node.esp32` via ESPNOW when the PIR sensor detects presence.
* **`board_status_updater`**: Sends node status information (reboot count, uptime since last reboot) to `gateway.node.esp32` via ESPNOW every minute.

---

### gateway.node.esp32

This node is also based on an **ESP32 DevKit V1** and acts as the entry/exit point for the local IoT network.

**Functionality:**
* **Synchronizes its internal RTC with a public NTP server via WiFi** upon startup.
* Responds to **clock synchronization requests** from other ESPNOW nodes using its internal RTC.
* **Publishes data received from local IoT sensor nodes to the appropriate MQTT broker event channels.**
* Leverages **FreeRTOS tasks** for concurrency.

**Mandatory FreeRTOS Tasks:**
* **`internal_RTC_updater`**: Synchronizes the internal RTC with a public NTP server via WiFi.
* **`temperature_humidity_updater`**: Publishes "temperature" and "humidity" events received from local IoT nodes to the MQTT broker.
* **`analog_potentiometer_updater`**: Publishes "potentiometer" events received from local IoT nodes to the MQTT broker.
* **`presence_updater`**: Publishes "presence" events received from local IoT nodes to the MQTT broker.
* **`board_status_updater`**: Publishes `gateway.node.esp32` status information (reboot count, uptime since last reboot) to the MQTT broker every minute.

---

### MQTT Broker (Mosquitto on Raspberry Pi)

The MQTT broker is an **Eclipse Mosquitto Docker container** managed with **Docker Compose**, running on a **Raspberry Pi** accessible via SSH. This deployment is handled by the `rpi-iot.server` repository.

**Key Requirements:**
* **Automated Deployment**: The entire deployment must be automated using appropriate tools (scripts, configuration files). All necessary materials (scripts, configuration files) are managed within the `rpi-iot.server` repository using deployment keys.
* **Authentication**: The broker must use **authentication** for access to available channels.
* **Dedicated Event Channels**: A different event channel must exist for each data type, following this topic scheme:
    `/private_iot_network_id/data_type/node_id/`
    * `private_iot_network_id`: Identifier for the network where the event originated.
    * `data_type`: Type of data published (e.g., `humidity`, `temperature`, `presence`, `potentiometer`, `board_status`).
    * `node_id`: Identifier of the node that generated the event.

---

### Dummy Publisher

A Python 3 script that simulates the behavior of a `sensor.node.esp32` node.

**Functionality:**
* **Simulates sensor data** with random values:
    * **Temperature**: Random value between -5°C and 45°C every 5 seconds.
    * **Humidity**: Random value between 0% and 100% every 5 seconds.
    * **Presence Event**: Generated every X seconds, where X is a random value between 5 and 15 (X changes after each event).
    * **Potentiometer Change Event**: Generated every Y seconds, where Y is a random value between 5 and 15 (Y changes after each event). Potentiometer value is a random percentage between 0% and 100%.
* Accepts a numerical argument **`N`** from the command line, specifying the number of `sensor.node.esp32` nodes to simulate.
* Generates **independent random data** for each simulated `sensor.node.esp32`.
* Ensures **full concurrency** by creating three threads for *each* simulated node:
    * **Thread 1**: Sends MQTT events for temperature and humidity every 5 seconds.
    * **Thread 2**: Sends MQTT events for presence detection.
    * **Thread 3**: Sends MQTT events for potentiometer value changes.
* **Properly handles `Ctrl+C` signal** to ensure correct termination of all created threads.
* All script configuration parameters are externalized to a **configuration file**.

---

## Getting Started

### Hardware Prerequisites
* 1x Raspberry Pi (e.g., Raspberry Pi 4)
* 2x ESP32 DevKit V1 boards (one for sensor, one for gateway)
* 1x DHT11 Humidity/Temperature Sensor
* 1x PIR HC-SR501 Motion Sensor
* 1x Analog Potentiometer
* Jumper wires, breadboard, power supplies.

### Software Setup (Raspberry Pi Server)

1.  **SSH Access**: Ensure you can SSH into your Raspberry Pi.
    ```bash
    ssh pi@your_rpi_ip
    ```
2.  **Clone `rpi-iot.server`**: On your Raspberry Pi, clone the `rpi-iot.server` part of this repository.
    ```bash
    git clone [https://github.com/your-username/your-repo-name.git](https://github.com/your-username/your-repo-name.git)
    cd your-repo-name/rpi-iot.server
    ```
3.  **Docker & Docker Compose Installation**: If not already installed, use the deployment scripts in `deployment_scripts/` to install Docker and Docker Compose on your Raspberry Pi.
4.  **Mosquitto Deployment**: Navigate to `rpi-iot.server/` and deploy the Mosquitto broker using Docker Compose. Ensure `mosquitto_config/` contains the necessary authentication files.
    ```bash
    docker compose up -d
    ```
    Confirm the broker is running and accessible on port 1883 (or as configured).

### Firmware Flashing (ESP32 Nodes)

1.  **Install ESP-IDF**: Follow the official Espressif documentation to install ESP-IDF on your development machine.
2.  **Clone `iot.devices`**: Clone the `iot.devices` part of this repository to your development machine.
    ```bash
    git clone [https://github.com/your-username/your-repo-name.git](https://github.com/your-username/your-repo-name.git)
    cd your-repo-name/iot.devices
    ```
3.  **Compile & Flash `sensor.node.esp32`**:
    ```bash
    cd sensor.node.esp32
    idf.py set-target esp32
    idf.py menuconfig # Configure WiFi credentials, MQTT broker details, etc.
    idf.py build
    idf.py -p /dev/ttyUSB0 flash monitor # Adjust port as needed
    ```
4.  **Compile & Flash `gateway.node.esp32`**:
    ```bash
    cd ../gateway.node.esp32
    idf.py set-target esp32
    idf.py menuconfig # Configure WiFi credentials, MQTT broker details, etc.
    idf.py build
    idf.py -p /dev/ttyUSB1 flash monitor # Adjust port as needed
    ```
    **Note**: Ensure unique MAC addresses or ESPNOW peer configurations for communication.

### Running the Dummy Publisher

1.  **Navigate to `dummy_publisher`**:
    ```bash
    cd your-repo-name/iot.devices/dummy_publisher
    ```
2.  **Install Dependencies**:
    ```bash
    pip install paho-mqtt # Or other necessary libraries
    ```
3.  **Configure**: Edit the `config.ini` (or chosen config file) with your MQTT broker details (host, port, username, password) and network ID.
4.  **Run**:
    ```bash
    python3 dummy_publisher.py <N> # Replace <N> with the number of simulated nodes
    ```
    For example, to simulate 3 sensor nodes:
    ```bash
    python3 dummy_publisher.py 3
    ```

---

## Communication Protocols

* **ESPNOW**: Used for direct, low-power communication between `sensor.node.esp32` and `gateway.node.esp32` nodes. This protocol is ideal for battery-operated devices due to its connectionless nature.
* **WiFi**: Used by `gateway.node.esp32` to connect to the internet for NTP synchronization and to the local network for MQTT broker communication.
* **MQTT**: A lightweight messaging protocol used for publishing sensor data and node status from `gateway.node.esp32` and `dummy_publisher` to the central broker.
* **NTP**: Network Time Protocol used by `gateway.node.esp32` to synchronize its internal RTC with public time servers.

---

## Data Formats & Topics

All data sent to the MQTT broker will follow the specified topic structure:
`/private_iot_network_id/data_type/node_id/`

**Example Topics:**
* `/my_iot_net/temperature/sensor001/`
* `/my_iot_net/humidity/sensor001/`
* `/my_iot_net/presence/sensor002/`
* `/my_iot_net/potentiometer/sensor003/`
* `/my_iot_net/board_status/gateway001/`

**Message Payloads:**
* **Sensor Readings (Temperature, Humidity, Potentiometer)**: JSON objects containing `value` and `timestamp_utc`.
    Example: `{"value": 25.4, "timestamp_utc": "2025-07-07T10:30:00Z"}`
* **Presence Detection**: JSON object indicating presence status and timestamp.
    Example: `{"presence": true, "timestamp_utc": "2025-07-07T10:31:05Z"}`
* **Board Status**: JSON object containing `reboot_count`, `uptime_seconds`, and `timestamp_utc`.
    Example: `{"reboot_count": 5, "uptime_seconds": 3600, "timestamp_utc": "2025-07-07T10:32:15Z"}`

Data sent via ESPNOW between ESP32 nodes will require custom binary or serialized formats, ensuring efficiency for batch transmission and parsing timestamps.

---

## Concurrency Model (FreeRTOS)

Both `sensor.node.esp32` and `gateway.node.esp32` firmly utilize **FreeRTOS** for concurrent operation, taking full advantage of the ESP32's dual cores. Tasks are designed to run independently, communicating via FreeRTOS mechanisms such as **queues**, **semaphores**, and **event groups** to ensure proper synchronization and data flow. This approach guarantees responsiveness and efficient resource management.
