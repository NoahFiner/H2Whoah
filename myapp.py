from flask import Flask, Response, request, render_template, jsonify, redirect
from werkzeug.serving import WSGIRequestHandler
import influxdb

app = Flask(__name__)

host = 'localhost'
username = 'h2woah'
password = 'asdfasdf'
database = 'H2WOAH'
client = influxdb.InfluxDBClient(host=host, username=username,
                                 password=password, database=database)

#@app.route('/last/<lab_group>')
#def get_prev_data(number):
#    query = client.query("SELECT last(value) from /d[0-9]/ WHERE lab_group='{0}'".format(lab_group)).raw
#    result = {}
#    if 'series' in query:
#        query = query['series']
#        for point in query:
#            result.update({point['name'] : point['values'][-1][-1]})
#    return jsonify(**result)

@app.route('/<amount>')
def show_index():
    query = client.query("SELECT * FROM \"h2whoah\" ORDER BY DESC LIMIT %s".format(amount).raw
    return render_template('main.html', data=raw)

# @app.route('/submit' , methods = ['POST'])
#def submit_command():
#    entered_id = request.form['id']
#    entered_state = request.form['value']
#    entered_group = request.form['lab_group']
#    command = ["{0},lab_group={1} value={2}".format(entered_id, entered_group, entered_state)]
#    client.write_points(command, protocol='line')
#    return 'OK'

if __name__ == "__main__":
    WSGIRequestHandler.protocol_version = "HTTP/1.1"
    app.run(host='0.0.0.0', port=5000)