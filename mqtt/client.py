# client.py
import paho.mqtt.client as mqtt
import numpy as np
from queue import Queue

# === Shared Queue to send data to Flask ===
position_queue = Queue()

# MQTT Settings
BROKER = "192.168.137.1"
TOPICS = ["BLE","WIFI"]


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
    
TAG_MACS ={
    "BLE_PRED":"D4:D4:DA:85:31:2A",
    "WIFI_PRED":"D4:D4:DA:85:31:28"
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
    global rssi_data
    global BLE_MACS
    global WIFI_MACS
    payload = msg.payload.decode()
    try:
        print(payload)
        source = msg.topic  # "BLE" or "WIFI"
        beacon_mac, tag_mac, distance = payload.split(",")
        distance = float(distance)

        beacon_id = None
        renamed_beacon_mac = None

        beacon_mac = beacon_mac.strip().upper()  # Remove any extra spaces from beacon_mac
        tag_mac = tag_mac.strip().upper()  # Optionally normalize tag_mac too

        
        if source == "BLE":
            for mac in BLE_MACS.values():
                if beacon_mac.strip().upper() == mac.strip().upper():
                    beacon_id = list(BLE_MACS.keys())[list(BLE_MACS.values()).index(mac)]
                    renamed_beacon_mac = mac
                    break
            else:
                print(f"Invalid BLE beacon_mac: {beacon_mac}")
                return

        if source == "WIFI":
            normalized_beacon_mac = beacon_mac.strip().replace(":", "").upper()
            found = False
            for mac in WIFI_MACS.values():
                normalized_mac = mac.strip().replace(":", "").upper()
                print(f"Normalized MAC - beacon_mac: {normalized_beacon_mac}, mac: {normalized_mac}")
                if normalized_beacon_mac == normalized_mac:
                    beacon_id = list(WIFI_MACS.keys())[list(WIFI_MACS.values()).index(mac)]
                    renamed_beacon_mac = mac
                    found = True
                    break
            if not found:
                print(f"Invalid WIFI beacon_mac: {beacon_mac}")
                return
            
            print(f"beacon_id found: {beacon_id}")

            # Ensure that the beacon_id exists in WIFI_MACS
            if beacon_id not in WIFI_MACS:
                print(f"KeyError: {beacon_id} not found in WIFI_MACS")
                return


        if beacon_id:
            rssi_data[beacon_id][tag_mac] = distance

        if all(tag_mac in rssi_data[b] for b in beacon_positions):
            print("beacon id")
            tag_distances = {b: rssi_data[b][tag_mac] for b in beacon_positions if tag_mac in rssi_data[b]}
            tag_position = trilateration(beacon_positions, tag_distances)

            if tag_position is not None:
                x, y, z = round(tag_position[0], 2), round(tag_position[1], 2), round(tag_position[2], 2)

                ble_actual_distances = {
                    b_id: calculate_distance((x, y, z), pos)
                    for b_id, pos in beacon_positions.items()
                }
                wifi_actual_distances = {
                    w_id: calculate_distance((x, y, z), pos)
                    for w_id, pos in wifi_positions.items()
                }

                closest_ble = min(ble_actual_distances.items(), key=lambda x: x[1])
                closest_wifi = min(wifi_actual_distances.items(), key=lambda x: x[1])

                position_queue.put({
                    "id": tag_mac,
                    "x": x,
                    "y": y,
                    "z": z,
                    "closest_ble_mac": renamed_beacon_mac,
                    "closest_ble_distance": round(closest_ble[1], 2),
                    "closest_wifi_mac": closest_wifi[0],
                    "closest_wifi_distance": round(closest_wifi[1], 2)
                })

                rssi_data = {b: {} for b in beacon_positions}  # Reset rssi_data for next tag

    except ValueError as e:
        print(f"ValueError processing message: {e}")
    except KeyError as e:
        print(f"KeyError processing message: {e}")
    except Exception as e:
        print(f"Error processing message: {e}")


def start_mqtt():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)

    for topic in TOPICS:
        client.subscribe(topic)

    client.loop_forever()
