# Low-Cost-IOT-Asset-Tracking

# MQTT Broker
cd mos2
mosquitto -c mosquitto.conf -v

# Beacon
change #define BEACON_NAME to "BEACON1" / "BEACON2" / "BEACON3"
flash beacon.ino

# Asset Tag
flash asset_tag.ino

# Python Client
cd mqtt
pip install -r requirements.txt
python client.py