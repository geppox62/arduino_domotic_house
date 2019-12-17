# Fishino "json"

This project is derived from Arduino_domotic_house main project, with two differences:
- instead of using an Arduino Mega, it uses Fishino Mega
- data packets between the browser and Fishino are in json format

With Fishino (and Fishino Mega, of course), we can define a standalone wi-fi network and use it instead uf using an ethernet cable (and relative ethernet shield used with Arduino Mega).
I called standalone wi-fi network "MyFishino" without password: this can be changed at lines 103-104 of the sketch.
