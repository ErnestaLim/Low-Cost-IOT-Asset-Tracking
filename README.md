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

# Testing:
mosquitto_sub -h <ip> -p 8883 -u beacon -P securepassword -t test -v -d --cafile ca.crt

# Mosquito Cred 
mosquitto_passwd -c <file> <password>

Cert Generation:
# Generate CA
openssl genrsa -out ca.key 2048
openssl req -new -key client.key -out client.csr -config client.cnf
openssl req -new -x509 -days 365 -key ca.key -out ca.crt

# Generate server cert
openssl req -new -out server.csr -key server.key -config server.cnf
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -extensions v3_req -extfile server.cnf