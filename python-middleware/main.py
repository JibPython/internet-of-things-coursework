# Example of using the MQTT client class to subscribe to a feed and print out
# any changes made to the feed.  Edit the variables below to configure the key,
# username, and feed to subscribe to for changes.
import os
import sys

from Adafruit_IO import MQTTClient
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
from google.cloud.firestore import SERVER_TIMESTAMP

cred = credentials.Certificate("./firebase-key.json")
firebase_admin.initialize_app(cred)

db = firestore.client()

username = os.getenv('ADAFRUIT_IO_USERNAME')

key = os.getenv('ADAFRUIT_IO_KEY')

# JibPython/feeds/alert
feed = 'alert'

# Define callback functions which will be called when certain events happen.
def connected(client):
    # Connected function will be called when the client is connected to Adafruit IO.
    # This is a good place to subscribe to feed changes. The client parameter
    # passed to this function is the Adafruit IO MQTT client so you can make
    # calls against it easily.
    print(
        "Connected to Adafruit IO as {0}!  Listening for {1} changes...".format(
            username, feed
        )
    )
    client.subscribe(feed)

def subscribe(client, userdata, mid, granted_qos):
    # This method is called when the client subscribes to a new feed.
    print('Subscribed to {0} with QoS {1}'.format(feed, granted_qos[0]))

def disconnected(client):
    # Disconnected function will be called when the client disconnects.
    print('Disconnected from Adafruit IO!')
    sys.exit(1)

def message(client, feed_key, payload):
    # Message function will be called when a subscribed feed has a new value.
    # The feed_key parameter identifies the feed, and the payload parameter has
    # the new value.
    print('Feed {0} received new value: {1}'.format(feed_key, payload))

    save_to_database(payload)

def save_to_database(msg):
    doc_details = {
        "Date": SERVER_TIMESTAMP,
        "Message": msg,
    }

    doc_ref = db.collection("alerts")
    doc_ref.add(doc_details)

    print("doc_ref id is :", doc_ref.id)


client = MQTTClient(username, key)

client.on_connect = connected
client.on_disconnect = disconnected
client.on_message = message
client.on_subscribe = subscribe

client.connect()

# Start a message loop that blocks forever waiting for MQTT messages to be
# received.  Note there are other options for running the event loop like doing
# so in a background thread--see the mqtt_client.py example to learn more.
client.loop_blocking()