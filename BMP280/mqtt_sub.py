import paho.mqtt.client as mqtt  # Install paho-mqtt -- $pip install paho-mqtt
from datetime import datetime

# ==========================================
# MQTT Broker Configuration
# ==========================================

BROKER_IP = "192.168.1.4"
BROKER_PORT = 1883

# MQTT Topics
TOPICS = [
    ("sensor/bmp280/temperature", 0),
    ("sensor/bmp280/pressure", 0),
    ("sensor/bmp280/altitude", 0)
]

# ==========================================
# Callback Functions
# ==========================================

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("===================================")
        print("Connected to MQTT Broker")
        print("===================================")

        # Subscribe to topics
        client.subscribe(TOPICS)

        print("Subscribed Topics:")
        for topic, qos in TOPICS:
            print(f"  -> {topic}")

        print("===================================\n")

    else:
        print(f"Connection Failed! Error Code = {rc}")

# ==========================================

def on_message(client, userdata, msg):

    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    topic = msg.topic
    payload = msg.payload.decode()

    print("===================================")
    print(f"Timestamp : {timestamp}")
    print(f"Topic     : {topic}")
    print(f"Data      : {payload}")

    # Optional formatted output
    if "temperature" in topic:
        print(f"Temperature = {payload} °C")

    elif "pressure" in topic:
        print(f"Pressure    = {payload} hPa")

    elif "altitude" in topic:
        print(f"Altitude    = {payload} meters")

    print("===================================\n")

# ==========================================
# Main Program
# ==========================================

client = mqtt.Client()

# Attach Callbacks
client.on_connect = on_connect
client.on_message = on_message

print("Connecting to MQTT Broker...\n")

client.connect(BROKER_IP, BROKER_PORT, 60)

# Infinite loop for MQTT listening
client.loop_forever()
