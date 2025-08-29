require([
    "esri/Map",
    "esri/Graphic",
    "esri/geometry/Circle",
    "esri/views/MapView",
    "esri/widgets/CoordinateConversion",
    "esri/widgets/Locate"
    ], 
function (
    Map,
    Graphic,
    Circle,
    MapView,
    CoordinateConversion,
    Locate) {
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
        });
    }
);