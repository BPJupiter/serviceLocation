import subprocess
from services import *

class Traceroute:
    def __init__(self):
        self.addr = ""
        self.pingList = []
        self.nHops = 0

    def route(self, ipv4Addr):
        validate_ipv4(ipv4Addr)
        self.addr = ipv4Addr
        try:
            proc = subprocess.Popen(["sudo", "../static/c/ping.out", ipv4Addr, "1"], stdout=subprocess.PIPE)
            while True:
                s = str(proc.stdout.readline())
                if s == "b''":
                    break
                self.nHops += 1
                ps = s.find("MIN: ") + 5
                pe = s.find("ms")
                if ps == -1 or pe == -1:
                    self.pingList.append(-1)
                    continue
                self.pingList.append(int(s[ps:pe]))
            self.pingList.pop()

        except subprocess.CalledProcessError as e:
            print(f"Command failed with return code {e.returncode}")