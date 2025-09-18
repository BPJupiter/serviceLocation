import ipaddress

def validate_ip(addr):
    try:
        ipaddress.ip_address(addr)
    except ValueError:
        raise ValueError("Invalid IP Address")
    
def find_min_ping(line):
    parts = line.split(")")
    pingArr = []
    for i in range(1, len(parts)):
        pings = parts[i].split("  ")
        for j in range(1, len(pings)):
            pe = pings[j].find(" ms")
            ping = pings[j][:pe]
            pingArr.append(round(float(ping)))
    if len(pingArr) == 0:
        return -1
    return min(pingArr)

def find_address(line):
    ads = line.find("(") + 1
    ade = line.find(")")
    if ads == -1 or ade == -1:
        return ""
    else:
        return line[ads:ade]