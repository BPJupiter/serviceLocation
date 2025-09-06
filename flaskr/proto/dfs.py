import json
from services import *

with open('../static/json/cities-GCS.json', 'r') as file:
    data = json.load(file)

min = 21000
cities = list()
for i, feature in enumerate(data["features"]):
    lng, lat = feature["geometry"]["coordinates"]
    coords = (lat, lng)
    prop = feature["properties"]
    name = prop["Name"]
    id = prop["city_id"]
    city = City(id, name, lat, lng)
    cities.append(city)
cities.sort(key=great_circle_distance)
print(cities)
print(cities[0])
print(cities[1])