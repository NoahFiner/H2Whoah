from werkzeug.serving import WSGIRequestHandler
import influxdb
from datetime import datetime

host = 'localhost'
username = 'h2woah'
password = 'asdfasdf'
database = 'H2WOAH'
client = influxdb.InfluxDBClient(host=host, username=username,
                                 password=password, database=database)

# REQUIRES: height, moisture, result are FLOATS (not ints), type is "a" | "m"
# height, moisture, result must be 0.0 or 1.0, not 0 or 1
# EFFECTS: writes to database h2woah
# MODIFIES: none
def write_to_db(height, moisture, result, type):
    current_time = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
    json = [{
		"measurement": "h2woah",
		"time": current_time,
		"fields": {
			"moisture": moisture,
			"height": height,
			"result": result,
			"type": type
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

# REQUIRES: data is the result of get_data, field_name is moisture, height, result, or type
# EFFECTS: returns the value associated with the column for the FIRST point in data
# MODIFIES: none
def get_value(data, field_name):
    index = data["series"][0]["columns"].index(field_name)
    return data["series"][0]["values"][0][index]

if __name__ == "__main__":
    print("Hello world")
    write_to_db(0.5, 0.6, 0.7, "a")
    print("Done writing")
    result = get_data("SELECT * FROM \"h2woah\" ORDER BY DESC LIMIT 1")
    print(result)
    print(get_value(result, "type"))
