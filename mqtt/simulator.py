import paho.mqtt.client as mqtt
import numpy as np
import time
from queue import Queue

position_queue = Queue()

BROKER = "192.168.137.1"
TOPICS = ["BLE", "WIFI"]

beacon_positions = {
    "BEACON1": (0, 0, 0),
    "BEACON2": (5, 0, 0),
    "BEACON3": (0, 5, 0),
    "BEACON4": (2.5, 2.5, 0)
}

wifi_positions = {
    "WIFI1": (0, 0, 0),
    "WIFI2": (5, 0, 0),
    "WIFI3": (0, 5, 0),
    "WIFI4": (2.5, 2.5, 0)
}

rssi_data = {b: {} for b in beacon_positions}

def calculate_distance(p1, p2):
    return np.sqrt((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2 + (p1[2] - p2[2])**2)

def trilateration(beacon_positions, distances):
    if len(distances) < 4:
        return None

    (x1, y1, z1), d1 = beacon_positions["BEACON1"], distances["BEACON1"]
    (x2, y2, z2), d2 = beacon_positions["BEACON2"], distances["BEACON2"]
    (x3, y3, z3), d3 = beacon_positions["BEACON3"], distances["BEACON3"]
    (x4, y4, z4), d4 = beacon_positions["BEACON4"], distances["BEACON4"]

    A = np.array([
        [2 * (x2 - x1), 2 * (y2 - y1), 2 * (z2 - z1)],
        [2 * (x3 - x1), 2 * (y3 - y1), 2 * (z3 - z1)],
        [2 * (x4 - x1), 2 * (y4 - y1), 2 * (z4 - z1)]
    ])

    B = np.array([
        [d1**2 - d2**2 - x1**2 + x2**2 - y1**2 + y2**2 - z1**2 + z2**2],
        [d1**2 - d3**2 - x1**2 + x3**2 - y1**2 + y3**2 - z1**2 + z3**2],
        [d1**2 - d4**2 - x1**2 + x4**2 - y1**2 + y4**2 - z1**2 + z4**2]
    ])

    try:
        position = np.linalg.lstsq(A, B, rcond=None)[0]
        return position.flatten()
    except np.linalg.LinAlgError:
        return None
def handle_mqtt_position_updates():
    while True:
        tags = []

        for tag in tags:
            # 🧠 Correct calculation for BLE distance
            ble_actual_distances = {}
            for ble_id, ble_pos in beacon_positions.items():
                dist = calculate_distance(
                    (tag["x"], tag["y"], tag["z"]), ble_pos)
                ble_actual_distances[ble_id] = dist

            # 🧠 Correct calculation for WiFi distance
            wifi_actual_distances = {}
            for wifi_id, wifi_pos in wifi_positions.items():
                dist = calculate_distance(
                    (tag["x"], tag["y"], tag["z"]), wifi_pos)
                wifi_actual_distances[wifi_id] = dist

            # Find closest WiFi and BLE
            closest_wifi = min(
                wifi_actual_distances.items(), key=lambda x: x[1])
            closest_ble = min(ble_actual_distances.items(), key=lambda x: x[1])

            # Put into queue
            position_queue.put({
                "id": tag["id"],
                "x": tag["x"],
                "y": tag["y"],
                "z": tag["z"],
                "closest_wifi_mac": closest_wifi[0],
                "closest_wifi_distance": round(closest_wifi[1], 2),
                "closest_ble_mac": closest_ble[0],
                "closest_ble_distance": round(closest_ble[1], 2)
            })

        time.sleep(1)

def get_beacon_positions():
    return beacon_positions