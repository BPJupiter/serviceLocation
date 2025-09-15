import json
from services import *

def get_cities_within_radius(ping_ms):
    with open('./flaskr/static/json/cities-GCS.json', 'r') as file:
        data = json.load(file)
        radius_km = ping_ms * 200
        min = 21000
        cities = list()
        for i, feature in enumerate(data["features"]):
            lng, lat = feature["geometry"]["coordinates"]
            coords = (lat, lng)
            prop = feature["properties"]
            name = prop["Name"]
            id = prop["city_id"]
            city = City(id, name, lat, lng)
            if great_circle_distance(city) <= radius_km:
                cities.append(city)
        cities.sort(key=great_circle_distance)
        for city in cities:
            print(city)

get_cities_within_radius(30)