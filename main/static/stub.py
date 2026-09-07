#!/usr/bin/env python3
from flask import Flask, send_from_directory, request
from datetime import datetime, timedelta
from types import SimpleNamespace
from io import StringIO
import json
import random

app = Flask(__name__)

with open('serverstatus.json') as f:
    example = json.load(f)

state = SimpleNamespace()
state.bootTime = datetime.now()
state.pumpInterval = timedelta(3)
state.pumpDuration = 2000
state.prevPumpTime = state.bootTime - timedelta(1)
state.nextPumpTime = state.prevPumpTime + state.pumpInterval
state.numPumpEvents = 0
state.sensorData = example['sensorData'][:]
state.wifiMode = "STA"
state.wifiSsid = "PlantWaterNet"
state.wifiPassword = ""
state.wifiHostname = "PlantWaterOS"

logfile = StringIO()


def main():
    log("server started")
    app.run(debug=True)


def log(*text):
    print("{}:".format(format_time(datetime.now())), *text, file=logfile)
    print("{}:".format(format_time(datetime.now())), *text)


def format_time(dt):
    return dt.strftime("%Y-%m-%d %H:%M:%S")


@app.route("/")
@app.route("/index.html")
def index():
    return send_from_directory('.', 'index.html')


@app.route("/style.css")
def style():
    return send_from_directory('.', 'style.css')


@app.route("/hyperapp.js")
def js():
    return send_from_directory('.', 'hyperapp.js')


@app.route("/file/events.log")
def events_log():
    return "Server started"


@app.route("/file/sensor.log")
def sensor_log():
    return logfile.getvalue()


def server_info():
    return {
        # Subtract one hour for testing correct display in client:
        "serverTime": format_time(datetime.now() - timedelta(seconds=3600)),
        "uptime": int((datetime.now() - state.bootTime).total_seconds()),
        "prevPumpTime": format_time(state.prevPumpTime),
        "nextPumpTime": format_time(state.nextPumpTime),
        "pumpInterval": state.pumpInterval.total_seconds(),
        "pumpDuration": state.pumpDuration,
        "numPumpEvents": state.numPumpEvents,
        "sensorValue": random.randint(800, 1200),
        "temperature": random.uniform(19, 21),
        "sensorData": state.sensorData,
        "wifiMode": state.wifiMode,
        "wifiSsid": state.wifiSsid,
        "wifiHostname": state.wifiHostname,
    }


@app.route("/api/status")
def api_status():
    return server_info()


@app.route("/api/server/reboot")
def api_server_reboot():
    log("Reboot")
    return {}


@app.route("/api/server/time", methods=['POST'])
def api_server_time():
    data = request.form
    log("serverTime =", data.get("serverTime"))
    return server_info()


@app.route("/api/pump/activate")
def api_pump_activate():
    state.prevPumpTime = datetime.now()
    state.nextPumpTime = state.prevPumpTime + state.pumpInterval
    state.numPumpEvents += 1
    log("pump activated")
    return server_info()


@app.route("/api/pump/reset")
def api_pump_reset():
    state.nextPumpTime = datetime.now() + state.pumpInterval
    log("reset pump timer")
    return server_info()


@app.route("/prefs", methods=['POST'])
def prefs():
    data = request.form
    log("updating prefs ", data)

    nextPumpTime = data.get("nextPumpTime")
    if nextPumpTime is not None:
        state.nextPumpTime = datetime.strptime(
            nextPumpTime, "%Y-%m-%d %H:%M:%S")
        log("pumpInterval =", nextPumpTime)

    pumpInterval = data.get("pumpInterval")
    if pumpInterval is not None:
        state.pumpInterval = timedelta(0, int(pumpInterval))
        log("pumpInterval =", pumpInterval)

    pumpDuration = data.get("pumpDuration")
    if pumpDuration is not None:
        state.pumpDuration = int(pumpDuration)
        log("pumpDuration =", pumpDuration)

    wifiMode = data.get("wifiMode")
    if wifiMode:
        state.wifiMode = wifiMode
        log("wifiMode =", wifiMode)

    wifiSsid = data.get("wifiSsid")
    if wifiSsid:
        state.wifiSsid = wifiSsid
        log("wifiSsid =", wifiSsid)

    wifiPassword = data.get("wifiPassword")
    if wifiPassword:
        state.wifiPassword = wifiPassword
        log("wifiPassword =", wifiPassword)

    wifiHostname = data.get("wifiHostname")
    if wifiHostname:
        state.wifiHostname = wifiHostname
        log("wifiHostname =", wifiHostname)

    return server_info()


@app.route("/update", methods=['POST'])
def upload():
    data = request.form
    log("upload...", data)
    messages = ['Done!', 'Internal failure!']
    code = random.choice(range(len(messages)))
    return {
        "code": code,
        "message": messages[code],
    }


if __name__ == '__main__':
    main()
