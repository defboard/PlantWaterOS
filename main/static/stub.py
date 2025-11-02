#!/usr/bin/env python3
from flask import Flask, send_from_directory, request
from datetime import datetime, timedelta
from types import SimpleNamespace
import random

app = Flask(__name__)

state = SimpleNamespace()
state.bootTime = datetime.now()
state.pumpInterval = timedelta(3)
state.prevPumpTime = datetime(2000, 1, 1, 0, 0, 0)
state.nextPumpTime = state.prevPumpTime + state.pumpInterval
state.numPumpEvents = 0


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


def server_info():
    return {
        "serverTime": format_time(datetime.now()),
        "bootTime": format_time(state.bootTime),
        "localIP": "{}:{}".format(*request.server),
        "prevPumpTime": format_time(state.prevPumpTime),
        "nextPumpTime": format_time(state.nextPumpTime),
        "pumpInterval": state.pumpInterval.days,
        "numPumpEvents": state.numPumpEvents,
        "sensorValue": random.randint(800, 1200),
        "temperature": random.uniform(19, 21),
    }


@app.route("/api/status")
def api_status():
    return server_info()


@app.route("/api/pump/activate")
def api_pump_activate():
    state.prevPumpTime = datetime.now()
    state.nextPumpTime = state.prevPumpTime + state.pumpInterval
    state.numPumpEvents += 1
    return server_info()


@app.route("/api/pump/reset")
def api_pump_reset():
    state.nextPumpTime = datetime.now() + state.pumpInterval
    return server_info()


if __name__ == '__main__':
    app.run(debug=True)
