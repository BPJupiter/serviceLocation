require([
    "esri/Map",
    "esri/Graphic",
    "esri/geometry/Point",
    "esri/geometry/Circle",
    "esri/views/MapView",
    "esri/widgets/CoordinateConversion",
    "esri/widgets/Locate",
    "esri/form/FormTemplate"
    ], 
function (
    Map,
    Graphic,
    Point,
    Circle,
    MapView,
    CoordinateConversion,
    Locate) {
        $(document).ready(function() {
            'use strict';

            function rgb_spaced(n) {
                function get_r(p) {
                    if (p < 1/6 || p >= 5/6)
                        return 255
                    else if (p < 2/6)
                        return Math.round(-6*255*p + 2*255)
                    else if (p < 4/6)
                        return 0
                    else
                        return Math.round(6*255*p - 4*255)
                }
                function get_g(p) {
                    if (p < 1/6)
                        return Math.round(6*255*p)
                    else if (p < 3/6)
                        return 255
                    else if (p < 4/6)
                        return Math.round(-6*255*p + 4*255)
                    else
                        return 0
                }
                function get_b(p) {
                    if (p < 2/6)
                        return 0
                    else if (p < 3/6)
                        return Math.round(6*255*p - 2*255)
                    else if (p < 5/6)
                        return 255
                    else
                        return Math.round(-6*255*p + 6*255)
                }
                let colArr = new Array()
                for (let i = 0; i < n; i++) {
                    var p = i/n
                    colArr.push([get_r(p), get_g(p), get_b(p)])
                }
                return colArr
            }

            const map = new Map({
                basemap: "dark-gray",
            });

            const view = new MapView({
                container: "viewDiv",
                map: map,
            });

            let ccWidget = new CoordinateConversion({
                view: view
            });

            view.ui.add(ccWidget, "bottom-left");

            let locateWidget = new Locate({
                view: view,
                graphic: new Graphic({
                    symbol: {type: "simple-marker"}
                })
            });

            view.ui.add(locateWidget, "bottom-left");
            $('#search').keypress(function (e) {
                if (e.which !== 13) {
                    return;
                }

                e.preventDefault();

                var search = $('#search').val().trim();
                if (!search) {return;}
                let ip = search
                fetch('http://localhost:5000/ping?ip='+ip)
                .then((response) => {
                    return response.json();
                })
                .then((pingJson) => {
                    console.log(pingJson.addresses)
                    console.log(pingJson.latencies)

                    const nHops = pingJson.addresses.length
                    let colourArr = rgb_spaced(nHops)

                    let relLatencies = new Array()
                    relLatencies.push(pingJson.latencies[0])
                    for (let i = 1; i < nHops; i++) {
                        var latency = pingJson.latencies[i] - pingJson.latencies[i-1]
                        relLatencies.push(Math.abs(latency))
                    }
                    console.log(relLatencies)
                    fetch('http://localhost:5000/locate?ipArr='+pingJson.addresses)
                    .then((response) => {
                        return response.json();
                    })
                    .then((coordJson) => {
                        console.log(coordJson.coords)
                        
                        let j = 0 //index to colourArr
                        for (let i = 0; i < nHops; i++) {
                            if (coordJson.coords[i][0] == null) {
                                continue
                            }
                            if (pingJson.latencies[i] == -1) { //redundant comparison
                                continue
                            }

                            const point = new Point({
                                longitude: coordJson.coords[i][0],
                                latitude: coordJson.coords[i][1]
                            });

                            view.graphics.add(new Graphic({
                                geometry: point,
                                symbol: {
                                    type: "simple-marker",
                                    color: colourArr[j],
                                    outline: {
                                        color: "white",
                                        width: 1,
                                    },
                                }
                            }));
                            
                            if (relLatencies[i] == 0) {
                                j++
                                continue
                            }

                            let centre = coordJson.coords[i]
                            let radii = relLatencies[i] * 200 // speed of light in fibreoptic is 200 km/ms
                            const circle = new Circle({
                                center: centre,
                                geodesic: true,
                                numberOfPoints: 100,
                                radius: radii,
                                radiusUnit: "kilometers"
                            });

                            view.graphics.add(new Graphic({
                                geometry: circle,
                                symbol: {
                                    type: "simple-fill",
                                    style: "none",
                                    outline: {
                                        width: 3,
                                        color: colourArr[j]
                                    }
                                }
                            }));
                            j++
                        }
                    })
                })
            });
        });
    }
);