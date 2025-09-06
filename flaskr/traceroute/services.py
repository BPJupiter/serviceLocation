import ipaddress

def validate_ipv4(addr):
    try:
        ipaddress.ip_address(addr)
    except ValueError:
        raise ValueError("Invalid IPv4 Address")