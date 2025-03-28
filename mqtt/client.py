# client.py
import paho.mqtt.client as mqtt
import numpy as np
from queue import Queue

# === Shared Queue to send data to Flask ===
position_queue = Queue()

# MQTT Settings
BROKER = "192.168.20.175"
TOPICS = ["BEACON1", "BEACON2", "BEACON3", "BEACON4"]
TAG_TOPIC = "TAG_POSITION"

beacon_positions = {
    "BEACON1": (0, 0, 0),
    "BEACON2": (1, 0, 0),
    "BEACON3": (0, 1, 0),
    "BEACON4": (0.5, 0.5, 1)
}

rssi_data = {beacon: {} for beacon in beacon_positions}


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
        x, y, z = position.flatten()
        return [x, y, z]
    except np.linalg.LinAlgError:
        return None


def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    try:
        source = msg.topic
        target, distance = payload.split(":")
        distance = float(distance)

        if source in rssi_data:
            rssi_data[source][target] = distance

        if all("TAG" in rssi_data[b] for b in beacon_positions):
            tag_distances = {b: rssi_data[b]["TAG"] for b in beacon_positions}
            tag_position = trilateration(beacon_positions, tag_distances)

            if tag_position:
                position_queue.put({
                    "x": round(tag_position[0], 2),
                    "y": round(tag_position[1], 2),
                    "z": round(tag_position[2], 2)
                })

    except ValueError:
        pass  # ignore bad messages


def start_mqtt():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)

    for topic in TOPICS:
        client.subscribe(topic)

    client.loop_forever()
