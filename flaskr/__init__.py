from flask import Flask, request, render_template
from flask import jsonify
from flaskr.traceroute.ping import Traceroute, get_coords

def create_app():
    app = Flask(__name__)

    @app.route('/')
    def home():
        return render_template('index.html')
    
    @app.route("/ping")
    def ping():
        ip = request.args.get('ip')
        type = request.args.get('type')
        tr = Traceroute()
        tr.route(ip, type)
        return jsonify({"addresses": tr.addrList,
                        "latencies": tr.pingList})

    @app.route("/locate")
    def locate():
        ipArr = request.args.get('ipArr')
        ipArr = ipArr.split(',')
        coordArr = list()
        for ip in ipArr:
            coordArr.append(get_coords(ip))
        coordArr[0] = get_coords("me")
        return jsonify({"coords": coordArr})

    return app