from flask import Flask, Response, request, render_template, jsonify, redirect
from werkzeug.serving import WSGIRequestHandler
import influxdb
import json
from datetime import datetime

app = Flask(__name__)

host = 'localhost'
username = 'h2woah'
password = 'asdfasdf'
database = 'H2WOAH'
client = influxdb.InfluxDBClient(host=host, username=username,
                                 password=password, database=database)

# REQUIRES: height, soil, new_weight are FLOATS (not ints), type is "a" | "m"
# height, height, soil, new_height must be 0.0 or 1.0, not 0 or 1
# input_time should either be the previous measurement's time (for overwriting)
# or datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ') (for a new measurement)
# EFFECTS: writes to database h2whoah
# MODIFIES: none
def write_to_db(height, soil, new_height, am_type, input_time):
    json = [{
                "measurement": "h2whoah",
                "time": input_time,
                "fields": {
                    "height": round(float(height), 5),
                    "soil": round(float(soil), 5),
                    "new_height": round(float(new_height), 5),
                    "value": am_type
                }
    }]
    print(json)
    client.write_points(json)

# REQUIRES: query is a valid influxDB query
# EFFECTS: returns raw data, read this through get_value
# MODIFIES: none
def get_data(query):
    print("Querying data with: " + query)
    return client.query(query).raw

# REQUIRES: data is the result of get_data, field_name is soil, height, new_height, or type
# EFFECTS: returns the value associated with the column for the FIRST point in data
# MODIFIES: none
def get_value(data, field_name):
    index = data["series"][0]["columns"].index(field_name)
    return data["series"][0]["values"][0][index]




@app.route('/show/<amount>')
def show_index(amount):
    query = client.query("SELECT * FROM \"h2whoah\" WHERE \"value\" = 'p' ORDER BY DESC LIMIT %s" % str(int(amount))).raw
    return render_template('index.html', data=query)

@app.route('/')
def show_default():
    query = client.query("SELECT * FROM \"h2whoah\" WHERE \"value\" = 'p' ORDER BY DESC LIMIT 20").raw
    return render_template('index.html', data=query)

@app.route('/override', methods=['POST'])
def override():
    if not request.json:
        print("ERROR")
    else:
        print(request.form["new_height"])
    new_height = request.form["new_height"]

    # get latest measurement
    result = get_data("SELECT * FROM \"h2whoah\" WHERE \"value\" = 'p' ORDER BY DESC LIMIT 1")
    # timestamp = get_value(result, "time")
    timestamp = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

    write_to_db(get_value(result, "height"), get_value(result, "soil"), new_height, "m", timestamp)

    return json.dumps(request.json)


if __name__ == "__main__":
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    app.run(host='0.0.0.0', port=5000)
