import subprocess
import geocoder
from flaskr.traceroute.services import *

class Traceroute:
    def __init__(self):
        self.addr = ""
        self.pingList = []
        self.addrList = []
        self.nHops = 0

    def route(self, ipv4Addr):
        validate_ipv4(ipv4Addr)
        self.addr = ipv4Addr
        try:
            proc = subprocess.Popen(["sudo", "./flaskr/static/c/ping.out", ipv4Addr, "1"], stdout=subprocess.PIPE)
            while True:
                s = str(proc.stdout.readline())
                if s == "b''":
                    break
                self.nHops += 1
                ps = s.find("MIN: ") + 5
                pe = s.find("ms")
                if ps == -1 + 5 or pe == -1:
                    self.pingList.append(-1)
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

        except subprocess.CalledProcessError as e:
            print(f"Command failed with return code {e.returncode}")

def get_coords(ipv4Addr):
    if ipv4Addr == "":
        return [None, None]
    coords = geocoder.ip(ipv4Addr)
    return [coords.lng, coords.lat]