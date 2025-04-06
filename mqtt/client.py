# client.py
import paho.mqtt.client as mqtt
import numpy as np
from queue import Queue
from scipy.optimize import least_squares

# === Shared Queue to send data to Flask ===
position_queue = Queue()

# MQTT Settings
BROKER = "192.168.20.175"
TOPICS = ["BLE", "WIFI"]

# === Separated beacon positions ===
BLE_POSITIONS = {
    "BEACON1": (0, 0, 0),
    "BEACON2": (5, 0, 0),
    "BEACON3": (0, 5, 0),
    "BEACON4": (2.5, 2.5, 3)
}

WIFI_POSITIONS = {
    "WIFI1": (0, 0, 0),
    "WIFI2": (5, 0, 0),
    "WIFI3": (0, 5, 0),
    "WIFI4": (2.5, 2.5, 3)
}

TAG_MACS = {
    "BLE_PRED": "D4:D4:DA:85:31:2A",
    "WIFI_PRED": "D4:D4:DA:85:31:28"
}

BLE_MACS = {
    "BEACON1": "4C:75:25:CB:9B:25",
    "BEACON2": "AC:0B:FB:6F:9E:89",
    "BEACON3": "4C:75:25:CB:8D:55",
    "BEACON4": "4C:75:25:CB:81:79"
}

WIFI_MACS = {
    "WIFI1": "4C:75:25:CB:9B:25",
    "WIFI2": "AC:0B:FB:6F:9E:89",
    "WIFI3": "4C:75:25:CB:8D:55",
    "WIFI4": "4C:75:25:CB:81:79"
}

# === Separate data stores for BLE/WIFI ===
ble_rssi = {beacon: {} for beacon in BLE_MACS}
wifi_rssi = {beacon: {} for beacon in WIFI_MACS}

def trilateration(positions, distances):
    def objective_function(point):
        return [np.linalg.norm(np.array(point) - np.array(pos)) - dist
               for pos, dist in zip(positions.values(), distances.values())]
    
    # Initial guess (centroid of beacons)
    initial_guess = np.mean(list(positions.values()), axis=0)
    
    # Set bounds based on your environment dimensions
    bounds = ([0, 0, 0], [5, 5, 3])  # Adjust based on your space
    
    result = least_squares(objective_function, 
                          initial_guess, 
                          bounds=bounds,
                          method='trf',
                          ftol=1e-4,
                          xtol=1e-6)
    
    return result.x.tolist() if result.success else None



def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    try:
        source = msg.topic
        beacon_mac, tag_mac, distance = payload.split(",")
        distance = float(distance)

        beacon_mac = beacon_mac.strip().upper()
        tag_mac = tag_mac.strip().upper()

        # === Separate processing for BLE/WIFI ===
        if source == "BLE":
            for bid, bmac in BLE_MACS.items():
                if beacon_mac == bmac.strip().upper():
                    ble_rssi[bid][tag_mac] = distance
                    if sum(tag_mac in ble_rssi[b] for b in BLE_MACS) >= 4:
                        distances = {b: ble_rssi[b][tag_mac] for b in BLE_MACS}
                        position = trilateration(BLE_POSITIONS, distances)
                        handle_position(tag_mac, position, distances, "BLE")
                    return

        elif source == "WIFI":
            for wid, wmac in WIFI_MACS.items():
                if beacon_mac == wmac.strip().upper():
                    wifi_rssi[wid][tag_mac] = distance
                    if sum(tag_mac in wifi_rssi[w] for w in WIFI_MACS) >= 4:
                        distances = {w: wifi_rssi[w][tag_mac] for w in WIFI_MACS}
                        position = trilateration(WIFI_POSITIONS, distances)
                        handle_position(tag_mac, position, distances, "WIFI")
                    return

        print(f"Unknown {source} beacon: {beacon_mac}")

    except Exception as e:
        print(f"Error processing message: {e}")

# client.py
def handle_position(tag_mac, position, distances, tech):
    if position is None:
        return

    x, y, z = round(position[0], 2), round(position[1], 2), round(position[2], 2)
    closest = min(distances.items(), key=lambda x: x[1])
    
    # Store tech-specific data
    update_data = {
        "id": tag_mac,
        "x": x,
        "y": y,
        "z": z,
        f"closest_{tech.lower()}_mac": closest[0],
        f"closest_{tech.lower()}_distance": round(closest[1], 2)
    }
    
    position_queue.put(update_data)


def start_mqtt():
    client = mqtt.Client()
    client.on_message = on_message
    client.tls_set(ca_certs="C:/Documents/IoT/Low-Cost-IOT-Asset-Tracking/mqtt/ca.crt")  # Path to CA certificate
    client.username_pw_set("beacon", "securepassword")
    client.connect("192.168.20.175", 8883, 60)
    for topic in TOPICS:
        client.subscribe(topic)
    client.loop_forever()