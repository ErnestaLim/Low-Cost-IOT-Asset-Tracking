# app.py
from flask import Flask, render_template, Response, jsonify
from threading import Thread
import time
from mqtt.client import start_mqtt, position_queue  # Use client's queue
from mqtt.simulator import handle_mqtt_position_updates, get_beacon_positions, wifi_positions

app = Flask(__name__)

latest_positions = {}

def update_position():
    while True:
        while not position_queue.empty():
            data = position_queue.get()
            tag_id = data.get("id", "UNKNOWN")
            
            # Ensure Z-value exists, default to 0 if missing
            if 'z' not in data:
                data['z'] = 0
                
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


@app.route('/wifi')
def get_wifi():
    return jsonify(wifi_positions)


if __name__ == "__main__":
    # Start MQTT listener in a background thread
    Thread(target=start_mqtt, daemon=True).start()
    Thread(target=handle_mqtt_position_updates, daemon=True).start()

    # Start update loop to sync queue to variable
    Thread(target=update_position, daemon=True).start()

    app.run(debug=True, port=5000)
