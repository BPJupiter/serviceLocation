from math import sin, cos, sqrt, atan2
import geopy.distance

import sys, os
current = os.path.dirname(os.path.realpath(__file__))
parent = os.path.dirname(current)
sys.path.append(parent)
from globals import SERV_COORD, OPTIC_SPEED

def great_circle_distance(lat, lng):
    coords1 = (SERV_COORD.lat, SERV_COORD.lng)
    coords2 = (lat, lng)

    return geopy.distance.geodesic(coords1, coords2).km

def great_circle_distance(city):
    coords1 = (SERV_COORD.lat, SERV_COORD.lng)
    coords2 = city.coords()

    return geopy.distance.geodesic(coords1, coords2).km

class City:
    def __init__(self, id, name, lat, lng):
        self.id = id
        self.name = name
        self.lat = lat
        self.lng = lng
    
    def coords(self):
        return self.lat, self.lng
    
    def dist(self, other):
        return geopy.distance.geodesic(self.coords, other.coords).km
    
    def __str__(self):
        return self.name
    
    def __repr__(self):
        return str(self.id)
    
    def __lt__(self, other):
        return great_circle_distance(self) < great_circle_distance(other)