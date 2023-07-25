import logging

from telegram import __version__ as TG_VER

try:
    from telegram import __version_info__
except ImportError:
    __version_info__ = (0, 0, 0, 0, 0)  # type: ignore[assignment]

if __version_info__ < (20, 0, 0, "alpha", 1):
    raise RuntimeError(
        # f"This example is not compatible with your current PTB version {TG_VER}. To view the "
        # f"{TG_VER} version of this example, "
        # f"visit https://docs.python-telegram-bot.org/en/v{TG_VER}/examples.html"
    )

import random

import numpy as np
import paho.mqtt.client as mqtt_client
import pandas as pd
from keras.models import load_model
from telegram import ForceReply, Update
from telegram.ext import (Application, CallbackContext, CommandHandler,
                          ContextTypes, JobQueue)
from tensorflow import keras

# Enable logging
logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)
logger = logging.getLogger(__name__)

# MQTT Settings
broker = 'armadillo.rmq.cloudamqp.com'
port = 1883
topicdata = "tfd/data"
topicloc = "tfd/location"
# Generate a Client ID with the publish prefix.
client_id = f'publish-{random.randint(0, 1000)}'
"""Load Model Once"""
model = load_model("modelnew.h5")
chat_ids = []
last_status = ''
lat = None
longi = None

client = mqtt_client.Client(client_id)


def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            logger.info("Connected to MQTT Broker!")
        else:
            logger.info("Failed to connect, return code %d\n", rc)

    # client = mqtt_client.Client(client_id)
    client.username_pw_set("**********",
                           "********************************")
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe_mqtt(client: mqtt_client):
    def on_message(client, userdata, msg):
        global last_status
        global lat, longi
        logger.info(
            f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
        # Convert the MQTT payload
        x = convert_mqtt_payload(msg.payload.decode())
        logger.info(x)
        lat = x[6]
        longi = x[7]
        status = predict(x[:6])  # Get the prediction status
        logger.info(f"status rn `{status}`")
        last_status = status

    client.subscribe(topicdata)
    # client.subscribe(topicloc)
    client.on_message = on_message


def convert_mqtt_payload(payload):
    """Function to convert MQTT payload to prediction input"""
    data = payload.split("/")  # Split the payload by slash
    values = [float(item.split(":")[1])
              for item in data]  # Extract the values after the colon
    return values

# Define a few command handlers. These usually take the two arguments update and
# context.


async def start(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Send a message when the command /start is issued."""
    user = update.effective_user
    await update.message.reply_html(
        rf"Hi {user.mention_html()}! Click 'Menu' button for more information. ",
        reply_markup=ForceReply(selective=True),
    )


async def help_command(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Send a message when the command /help is issued."""
    await update.message.reply_text("The following is a list of commands used in the Fall Detection with CNN bot: \n /start - To start this bot \n /subscribe - To get fall warning from now \n /unsubscribe - To temporarily pause fall warning \n /help - To display a list of commands and help information \n\n If need further assistance, please contact the developer.")


async def subscribe(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Subscribe if sensor online"""
    global chat_ids
    logger.info(update.effective_user)
    chat_ids.append(update.message.chat_id)
    await update.message.reply_text(rf"Hello {update.effective_user.name}, you will get fall warning from now!")


async def unsubscribe(update: Update, context: ContextTypes.DEFAULT_TYPE) -> None:
    """Subscribe if sensor online"""
    global chat_ids
    logger.info(update.effective_user)
    chat_ids.remove(update.message.chat_id)
    await update.message.reply_text(rf"Thank you {update.effective_user.name} for using this bot. Select subscribe to get fall warning again!")


def indextolabel(index):
    """Translate index to status message"""
    # works for single index
    label = ['Stand', 'Fall', 'On Floor']
    return label[index[0]]


def predict(x):
    """Function to predict status"""
    # Prepare the data for prediction
    test = pd.DataFrame([x], columns=['Ax', 'Ay', 'Az', 'Gx', 'Gy', 'Gz'])
    # Load the model
    # model = load_model("model1.h5")
    # Perform the prediction
    y_pred = np.argmax(model.predict(test), axis=-1)

    return indextolabel(y_pred)


async def send_warning_to_chat(context: CallbackContext) -> None:
    # async def send_warning_to_chat(bot) -> None:
    """Sends a warning to a specific chat ID using the Telegram bot."""
    global last_status
    global lat, longi

    location = f"https://www.google.com/maps/search/?api=1&query={lat:.6f},{longi:.6f}"

    bot = context.bot

    if last_status != '':
        if last_status == 'Fall':
            for chat_id in chat_ids:
                await bot.send_message(chat_id=chat_id, text=f"Warning! Detected {last_status} on {location}")
            last_status = ''
        else:
            for chat_id in chat_ids:
                await bot.send_message(chat_id=chat_id, text=f"Normal activity. Detected on {location}")
            last_status = ''
    # logger.info(last_status)


def main() -> None:
    """Start connection to MQTT sensor"""
    client = connect_mqtt()
    client.loop_start()

    """Start the bot."""
    # Create the Application and pass it your bot's token.
    job_queue = JobQueue()
    application = Application.builder().token(
        "**************************").job_queue(job_queue).build()

    subscribe_mqtt(client)
    application.add_handler(CommandHandler("start", start))
    application.add_handler(CommandHandler("help", help_command))
    application.add_handler(CommandHandler("subscribe", subscribe))
    application.add_handler(CommandHandler("unsubscribe", unsubscribe))

    job_interval = 1  # Interval in seconds
    job_queue.run_repeating(send_warning_to_chat,
                            interval=job_interval, first=0)

    # Run the bot until the user presses Ctrl-C
    # client.loop_stop()
    application.run_polling()


if __name__ == "__main__":
    main()
