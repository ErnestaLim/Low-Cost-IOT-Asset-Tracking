import paho.mqtt.client as mqtt
import numpy as np

# MQTT Settings
BROKER = "192.168.20.175"
TOPICS = ["BEACON1", "BEACON2", "BEACON3"]
TAG_TOPIC = "TAG_POSITION"

# Predefined beacon positions (adjust as needed)
beacon_positions = {
    "BEACON1": (0, 0),
    "BEACON2": (0, 0),
    "BEACON3": (0, 0)
}

# Dictionary to store distances
rssi_data = {
    "BEACON1": {},
    "BEACON2": {},
    "BEACON3": {}
}

def trilateration(beacon_positions, distances):
    if len(distances) < 3:
        print("Not enough beacons for trilateration")
        return None  # Need at least three distances

    # Extract beacon positions and distances
    (x1, y1), d1 = beacon_positions["BEACON1"], distances["BEACON1"]
    (x2, y2), d2 = beacon_positions["BEACON2"], distances["BEACON2"]
    (x3, y3), d3 = beacon_positions["BEACON3"], distances["BEACON3"]

    # Trilateration equations (linearized form)
    A = np.array([
        [2*(x2 - x1), 2*(y2 - y1)],
        [2*(x3 - x1), 2*(y3 - y1)]
    ])
    
    B = np.array([
        [d1**2 - d2**2 - x1**2 + x2**2 - y1**2 + y2**2],
        [d1**2 - d3**2 - x1**2 + x3**2 - y1**2 + y3**2]
    ])

    try:
        position = np.linalg.lstsq(A, B, rcond=None)[0]
        x, y = position.flatten()
        return [x, y]
    except np.linalg.LinAlgError:
        return None

def averaged_distances():
    # Calculate average distance between BEACON1 and BEACON2
    avg_dist = (rssi_data["BEACON1"]["BEACON2"] + rssi_data["BEACON2"]["BEACON1"]) / 2
    beacon_positions["BEACON2"] = (avg_dist, 0)
    # beacon_positions["BEACON2"] = (rssi_data["BEACON1"]["BEACON2"], 0)
    # Calculate average distance between BEACON1 and BEACON3
    avg_dist = (rssi_data["BEACON1"]["BEACON3"] + rssi_data["BEACON3"]["BEACON1"]) / 2
    beacon_positions["BEACON3"] = (0, avg_dist)
    # beacon_positions["BEACON3"] = (0,rssi_data["BEACON1"]["BEACON3"])

def print_beacon_positions():
    """Prints the current beacon positions."""
    print("Current Beacon Positions:")
    for beacon, position in beacon_positions.items():
        print(f"  {beacon}: X={position[0]:.2f}, Y={position[1]:.2f}")


def on_message(client, userdata, msg):
    """
    Handles incoming MQTT messages.
    Parses distances and computes the tag's position when sufficient data is collected.
    """
    payload = msg.payload.decode()
    # print(f"Received {msg.topic}:{payload}")

    try:
        source = msg.topic
        target, distance = payload.split(":")
        distance = float(distance)
        
        if source in rssi_data:
            rssi_data[source][target] = distance
            # print("added data")
        if "BEACON2" in rssi_data["BEACON1"] and "BEACON1" in rssi_data["BEACON2"] and "BEACON3" in rssi_data["BEACON1"] and "BEACON1" in rssi_data["BEACON3"]:
            averaged_distances()
            # If all beacons have distance to TAG, compute position
            if "TAG" in rssi_data["BEACON1"] and "TAG" in rssi_data["BEACON2"] and "TAG" in rssi_data["BEACON3"]:
                tag_distances = {b: rssi_data[b]["TAG"] for b in beacon_positions if "TAG" in rssi_data[b]}
                tag_position = trilateration(beacon_positions, tag_distances)
                if tag_position:
                    x, y = tag_position
                    print_beacon_positions()
                    print(f"Estimated Tag Position: X={x:.2f}, Y={y:.2f}")
                    # client.publish(TAG_TOPIC, f"{x:.2f},{y:.2f}")

    except ValueError:
        print("Invalid message format.")

def main():
    client = mqtt.Client()
    client.on_message = on_message
    client.connect(BROKER, 1883, 60)

    for topic in TOPICS:
        client.subscribe(topic)

    client.loop_forever()

if __name__ == "__main__":
    main()
