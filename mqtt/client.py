import paho.mqtt.client as mqtt
import numpy as np

# MQTT Settings
BROKER = "192.168.20.175"
TOPICS = ["BEACON1", "BEACON2", "BEACON3", "BEACON4"]
TAG_TOPIC = "TAG_POSITION"

# Predefined beacon positions (x, y, z)
beacon_positions = {
    "BEACON1": (0, 0, 0),
    "BEACON2": (1, 0, 0),
    "BEACON3": (0, 1, 0),
    "BEACON4": (0.5, 0.5, 1) 
}

# Dictionary to store distances
rssi_data = {beacon: {} for beacon in beacon_positions}


def trilateration(beacon_positions, distances):
    if len(distances) < 4:
        print("Not enough beacons for 3D trilateration")
        return None  # Need at least four distances

    # Extract positions and distances
    (x1, y1, z1), d1 = beacon_positions["BEACON1"], distances["BEACON1"]
    (x2, y2, z2), d2 = beacon_positions["BEACON2"], distances["BEACON2"]
    (x3, y3, z3), d3 = beacon_positions["BEACON3"], distances["BEACON3"]
    (x4, y4, z4), d4 = beacon_positions["BEACON4"], distances["BEACON4"]

    print(f"BEACON1: ({x1}, {y1}, {z1}), Distance={d1}")
    print(f"BEACON2: ({x2}, {y2}, {z2}), Distance={d2}")
    print(f"BEACON3: ({x3}, {y3}, {z3}), Distance={d3}")
    print(f"BEACON4: ({x4}, {y4}, {z4}), Distance={d4}")

    # Constructing system of equations for 3D trilateration
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


# def averaged_distances():
#     """Compute relative positions for BEACON2, BEACON3, and BEACON4."""
#     avg_dist = (rssi_data["BEACON1"]["BEACON2"] + rssi_data["BEACON2"]["BEACON1"]) / 2
#     beacon_positions["BEACON2"] = (avg_dist, 0, 0)

#     avg_dist = (rssi_data["BEACON1"]["BEACON3"] + rssi_data["BEACON3"]["BEACON1"]) / 2
#     beacon_positions["BEACON3"] = (0, avg_dist, 0)


def print_beacon_positions():
    """Prints the current beacon positions."""
    print("Current Beacon Positions:")
    for beacon, position in beacon_positions.items():
        print(f"  {beacon}: X={position[0]:.2f}, Y={position[1]:.2f}, Z={position[2]:.2f}")


def on_message(client, userdata, msg):
    """
    Handles incoming MQTT messages.
    Parses distances and computes the tag's position when sufficient data is collected.
    """
    payload = msg.payload.decode()

    try:
        source = msg.topic
        target, distance = payload.split(":")
        distance = float(distance)
        
        if source in rssi_data:
            rssi_data[source][target] = distance

        # Check if all beacon-to-beacon distances are available
        # if all(beacon in rssi_data["BEACON1"] for beacon in ["BEACON2", "BEACON3", "BEACON4"]):
        #     averaged_distances()

            # If all beacons have distance to TAG, compute position
        if all("TAG" in rssi_data[b] for b in beacon_positions):
            tag_distances = {b: rssi_data[b]["TAG"] for b in beacon_positions if "TAG" in rssi_data[b]}
            tag_position = trilateration(beacon_positions, tag_distances)

            if tag_position:
                x, y, z = tag_position
                print_beacon_positions()
                print(f"Estimated Tag Position: X={x:.2f}, Y={y:.2f}, Z={z:.2f}")
                # client.publish(TAG_TOPIC, f"{x:.2f},{y:.2f},{z:.2f}")

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
