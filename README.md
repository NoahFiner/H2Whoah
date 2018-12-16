# H2Whoah Smart Irrigation Barrel

## What is this?
This is the back end framework for a smart stormwater irrigation barrel, or a class project I went a little (actually, more like a lot) overboard on. This code helps analyze raw sensor measurements (height and water level) sent from a Particle Photon and, using Dark Sky's API, return a result for how long a valve should open to release a sufficient amount of water. Additionally, there will be a webapp using a Flask server to help plot sensor data and more.

![img_8436 2](https://user-images.githubusercontent.com/10334304/50049803-a1814a00-00bb-11e9-9dee-b450f596dc76.JPG)

See https://www.youtube.com/watch?v=7rMWMYl7sOk for a demo.

## Files
### script.ino
This file contains code for a Particle Photon to read measurements from an ultrasonic sensor and a soil moisture sensor. Additionally, the Photon can read and write to an InfluxDB hosted with AWS and communicate with the server-side script and web application.

This file uses
* C
* Photon HTTP requests
* InfluxDB API calls


### h2whoah-web/
This is the back-end for the Python Flask application, also containing templates. This page displays data measured by the Photon pulled from the InfluxDB database and also has a button to set a custom new water level through a POST request writing to the database.

This file uses
* Python Flask
* InfluxDB's Python library
* HTML/CSS/JS
* jQuery and Bootstrap
* Chart.js

### script.py
This is the server-side script that pulls weather data from Dark Sky's API and measurements from the database to calculate a new optimal water level. This is controlled by a cronjob to run every hour.

This file uses
* Python
* Python's Request library
* Dark Sky's API
* InfluxDB's Python library
* A crojob
