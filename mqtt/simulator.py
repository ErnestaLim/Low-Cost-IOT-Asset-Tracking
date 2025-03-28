# mqtt/simulator.py
import numpy as np
import time
from queue import Queue

position_queue = Queue()

beacon_positions = {
    "BEACON1": (0, 0, 0),
    "BEACON2": (1, 0, 0),
    "BEACON3": (0, 1, 0),
    "BEACON4": (0.5, 0.5, 1)
}


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


def simulate_dummy_data():
    while True:
        rssi_data = {
            "BEACON1": 0.7,
            "BEACON2": 0.7,
            "BEACON3": 0.7,
            "BEACON4": 1.0
        }

        tag_position = trilateration(beacon_positions, rssi_data)
        if tag_position is not None:
            x, y, z = tag_position
            # position_queue.put(
            #     {"x": round(x, 2), "y": round(y, 2), "z": round(z, 2)}
            # )

            position_queue.put(
                {
                    "id": "TAG1",
                    "x": 0.2,
                    "y": 0.3,
                    "z": 5
                },
            )
            position_queue.put(
                {
                    "id": "TAG2",
                    "x": 0.25,
                    "y": 0.42,
                    "z": 4
                },
            )
            position_queue.put(
                {
                    "id": "TAG3",
                    "x": 0.69,
                    "y": 0.42,
                    "z": 2
                },
            )
            position_queue.put(
                {
                    "id": "TAG4",
                    "x": 0.8,
                    "y": 0.7,
                    "z": 3
                },
            )
        time.sleep(1)  # simulate 1 reading per second


def get_beacon_positions():
    return beacon_positions
