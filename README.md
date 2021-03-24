# About

- dht22server.ino  
This application runs in NodeMCU with DHT22 sensor.  
It fetches date and time from NTP server and serves up-to-date sensor data on local webpage and api.

- writedb&#46;py  
This application fetches data from NodeMCU api and Open Weather Map and saves them to Heroku PostgreSQL database.
