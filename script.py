from werkzeug.serving import WSGIRequestHandler
import influxdb
from datetime import datetime
import requests

host = 'localhost'
username = 'h2woah'
password = 'asdfasdf'
database = 'H2WOAH'
client = influxdb.InfluxDBClient(host=host, username=username,
                                 password=password, database=database)

#dark sky secret key
dark_sky_secret_key = "95e17b6d64e8b1389322aa122f3a03d8"

#longitude and latitude of the north campus bell tower
aa_long_lat = "42.292080,-83.715855"
texas_long_lat = "31.796573, -106.420243"


# REQUIRES: JSON from a dark sky call (see https://darksky.net/dev/docs)
# percent chance of rain we're looking for. should be between 0 and 1.
# EFFECTS: return the amount of days until rain is predicted
# 0 means it's forecasted to rain today
# -1 means there is no rain forecasted for the next week
# MODIFIES: NONE
def get_days_until_rain(data, cutoff):
    result = -1
    i = 0
    for day in data["daily"]["data"]:
        # print(day["precipProbability"]) uncomment for testing
        if(day["precipProbability"] > cutoff):
            return i
        i += 1
    return result

#just like get_days_until_rain but with hours
def get_hours_until_rain(data, cutoff):
    result = -1
    i = 0
    for hour in data["hourly"]["data"]:
        # print(hour["precipProbability"]) uncomment for testing
        if(hour["precipProbability"] > cutoff):
            return i
        i += 1
    return result


# Prints the hours or days until rain in a location for testing purposes
def print_time_until_rain(data, cutoff):
    hours_until_rain = get_hours_until_rain(data, cutoff)
    if(hours_until_rain == -1):
        days_until_rain = get_days_until_rain(data, cutoff)
        if(days_until_rain == -1):
            print("No %.2f%% probability of rain in the next week" % cutoff)
        else:
            print("%d days until a %.2f%% chance of rain" % (days_until_rain, cutoff))
    else:
        print("%d hours until a %.2f%% chance of rain" % (hours_until_rain, cutoff))


# REQUIRES: height, soil, new_weight are FLOATS (not ints), type is "a" | "m"
# height, height, soil, new_height must be 0.0 or 1.0, not 0 or 1
# EFFECTS: writes to database h2whoah
# MODIFIES: none
def write_to_db(height, soil, new_height, am_type):
    current_time = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
    json = [{
		"measurement": "h2whoah",
                "time": current_time,
		"fields": {
                    "height": height,
		    "soil": soil,
		    "new_height": new_height,
		    "type": am_type
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

if __name__ == "__main__":
    # print("Hello world")
    long_lat = aa_long_lat #modify this to whatever long_lat you want
    r = requests.get("https://api.darksky.net/forecast/%s/%s" % (dark_sky_secret_key, long_lat))
    weather_json = r.json()
    print(weather_json)
    print_time_until_rain(weather_json, 0.75)
    write_to_db(0.5, 0.6, 0.7, "a")
    # print("Done writing")
    # result = get_data("SELECT * FROM \"h2whoah\" ORDER BY DESC LIMIT 1")
    # print(result)
    # print(get_value(result, "type"))
