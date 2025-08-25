require([
    "esri/Map",
    "esri/Graphic",
    "esri/geometry/Circle",
    "esri/views/MapView"
    ], 
function (
    Map,
    Graphic,
    Circle,
    MapView) {
        $(document).ready(function() {
            'use strict';

            const map = new Map({
                basemap: "dark-gray",
            });

            const view = new MapView({
                container: "viewDiv",
            map: map,
            });

            const circle = new Circle({
                center: [-113, 36],
                geodesic: true,
                numberOfPoints: 100,
                radius: 100,
                radiusUnit: "kilometers"
            });

            view.graphics.add(new Graphic({
                geometry: circle,
                symbol: {
                    type: "simple-fill",
                    style: "none",
                    outline: {
                        width: 3,
                        color: "red"
                    }
                }
            }))
        });
    }
);