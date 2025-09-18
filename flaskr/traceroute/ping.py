import subprocess
import geocoder
from flaskr.traceroute.services import *

class Traceroute:
    def __init__(self):
        self.addr = ""
        self.type = ""
        self.pingList = []
        self.addrList = []
        self.nHops = 0

    def route(self, ipAddr, type):
        validate_ip(ipAddr)
        self.addr = ipAddr
        if (type == "UDP"):
            pass
        elif (type == "ICMP_ECHO"):
            self.type = "-I"
        elif (type == "TCP_SYN"):
            self.type = "-T"
        else:
            pass
        try:
            if False:
                proc = subprocess.Popen(["sudo", "./flaskr/static/c/ping.out", ipAddr], stdout=subprocess.PIPE)
                while True:
                    s = str(proc.stdout.readline())
                    if s == "b''":
                        break
                    self.nHops += 1
                    ps = s.find("MIN: ") + 5
                    pe = s.find("ms")
                    if ps == -1 + 5 or pe == -1:
                        self.pingList.append(0)
                    else:
                        self.pingList.append(int(s[ps:pe]))

                    ads = s.find("from ") + 5
                    ade = s.find(" (")
                    if ads == -1 + 5 or ade == -1:
                        self.addrList.append("")
                    else:
                        self.addrList.append(s[ads:ade])
                self.pingList.pop()
                self.addrList.pop()
                return
            n_pings = 3
            command = ["sudo", "traceroute", ipAddr]
            if self.type != "":
                command.append(self.type)
            print(command)
            proc = subprocess.Popen(command, stdout=subprocess.PIPE)
            proc.stdout.readline() # Skip informational line
            while True:
                s = str(proc.stdout.readline())
                if s == "b''":
                    break
                self.nHops += 1
                self.pingList.append(find_min_ping(s))

                self.addrList.append(find_address(s))

        except subprocess.CalledProcessError as e:
            print(f"Command failed with return code {e.returncode}")

def get_coords(ipvAddr):
    if ipvAddr == "":
        return None
    coords = geocoder.ip(ipvAddr)
    return [coords.lng, coords.lat]