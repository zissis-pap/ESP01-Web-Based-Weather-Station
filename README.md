<h1>ESP01-WebBased-Weather-Station</h1>
<hr>
<figure>
  <img src = "https://user-images.githubusercontent.com/11696874/78469800-f15ea700-772c-11ea-9fcb-70e8d113f4eb.png">
  <figcaption> Weather Station web interface in action</figcaption>
</figure>

<h3>About this project</h3>
  <p> This project is identical to the Arduino-Web-Based-Weather Station but implemented with the ESP01 (ESP8266) microcontroller and a DHT11 Temperature/Humidity sensor. The DS1302 is not used but instead, the microcontroller fetches the time from an NTP Server.</p>
  <h4> System Function</h4>
  <p> The system fetches the date and time from a NTP Server and then it displays the data on the first table of the webpage along with the temperature and humidity measurements which are made every 2 seconds by the DHT11 sensor. The forementioned table is shown below.</p>
  <img src= "https://user-images.githubusercontent.com/11696874/78470346-6d5aee00-7731-11ea-907f-b30ef4c0a466.png">
  <p> Every 5 minutes the system stores the measured Temperature and Humidity and uses the data to calculate the average Temperature and Humidity values per hour. The calculations are displayed on the second table along with the corresponding hour. When the table is full, the system shifts the data left so up to 24 calculations may be displayed and retrieved.</p>
  <img src = "https://user-images.githubusercontent.com/11696874/78470478-9334c280-7732-11ea-8a5e-759969fea7df.png">
<p> When the day changes, the system calculates the daily average values of Temperature and Humidity using the data from the table above and the results are displayed on the third table. This table has the days prefixed so no data shifting is done when it is full. To clarify this, if a cell already contains data for a certain day e.g. last Monday and today is Monday, tomorrow the data in the Monday cells will be overwritten with the new data.</p>
  <img src = "https://user-images.githubusercontent.com/11696874/78470608-94b2ba80-7733-11ea-836d-97b41877a133.png">
<h4>Circuit</h4>
To be updated soon! Until then please connect the DHT11 sensor to the D02 pin of the microcontroller.


<h3>Required Libraries</h3>
<ul>
  <li><a href = "https://github.com/arduino-libraries/NTPClient">NPT Client Library</li>
  <li><a href = "https://github.com/adafruit/DHT-sensor-library">Adafruit DHT sensor library</li>
</ul>

<h3>Notes</h3>
<p>A function called SetIP() is implemented which allows setting the IP address of the system through the serial interface. This feature however is not yet used during runtime and may be removed in the future if no beneficial purpose for it emerge.</p> 
