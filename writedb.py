''' writedb.py reads data from NodeMCU and saves received data to database '''
import os
from sqlalchemy import create_engine, text
from dotenv import load_dotenv
import requests
import json

load_dotenv()

# Get sensor data from NodeMCU server
def get_sensorData():
    url = os.environ['NODE_URL']
    response = requests.request("GET", url)
    data = json.loads(response.text)
    return data

# Get sensor data from NodeMCU server
def get_OWMData():
    api_key = os.environ['OWM_APIKEY']
    city = os.environ['OWM_CITY']
    url = "http://api.openweathermap.org/data/2.5/weather?q=%s&units=metric&appid=%s" % (city,api_key)

    response = requests.get(url)
    data = json.loads(response.text)
    return data

# Get postgres URI
DATABASE_URL = os.environ['DATABASE_URL']

# Connect to DB
db = create_engine(DATABASE_URL)

# Print progress to console
print("Trying to save data to database!")
                
# Create SQL query using sqlalchemy params
values = get_sensorData()
owmvalues = get_OWMData()
sql = text('INSERT INTO datas(times, temperature, humidity, owmtemp, owmhum)  VALUES ( :time, :temperature, :humidity, :owmtemperature, :owmhumidity )')
result = db.execute(sql, {'time': values['time'], 'temperature': values['temperature'], 'humidity': values['humidity'], 'owmtemperature': owmvalues['main']['temp'], 'owmhumidity': owmvalues['main']['humidity']})
# Check that DB query bears fruit
if result == False:
    print("Could not save this row")
else:
    # Print progress to console
    print("Saved data to database!")