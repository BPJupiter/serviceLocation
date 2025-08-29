import ipaddress
import geocoder

OPTIC_SPEED = 200000000 #m/s
SERV_COORD = geocoder.ip('me')
# .lat and .lng

def validate_ipv4(addr):
    try:
        ipaddress.ip_address(addr)
    except ValueError:
        raise ValueError("Invalid IPv4 Address")