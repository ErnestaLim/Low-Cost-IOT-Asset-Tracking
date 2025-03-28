# app.py
from flask import Flask, render_template, Response, jsonify
from threading import Thread
import time
# from mqtt.client import start_mqtt, position_queue  # import shared queue
from mqtt.simulator import simulate_dummy_data, position_queue
from mqtt.simulator import get_beacon_positions

app = Flask(__name__)

latest_positions = {}


def update_position():
    while True:
        while not position_queue.empty():
            data = position_queue.get()
            tag_id = data.get("id", "UNKNOWN")
            latest_positions[tag_id] = data
        time.sleep(0.1)


@app.route("/")
def dashboard():
    return render_template("dashboard.html")


@app.route("/positions")
def get_positions():
    return jsonify(list(latest_positions.values()))


@app.route("/beacons")
def beacons():
    return jsonify(get_beacon_positions())


if __name__ == "__main__":
    # Start MQTT listener in a background thread
    # Thread(target=start_mqtt, daemon=True).start()
    Thread(target=simulate_dummy_data, daemon=True).start()

    # Start update loop to sync queue to variable
    Thread(target=update_position, daemon=True).start()

    app.run(debug=True, port=5000)
