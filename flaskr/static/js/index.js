$(document).ready(function() {
    const map = new maplibregl.Map({
        container: 'viewDiv',
        style: 'https://basemaps.cartocdn.com/gl/dark-matter-nolabels-gl-style/style.json',
        center: [0,0],
        zoom: 1
    });
    load_images();
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

                    draw_city(coordJson.coords[i], i);
                    
                    if (relLatencies[i] == 0) {
                        j++
                        continue
                    }

                    draw_circle(coordJson.coords[i], relLatencies[i] * 200, colourArr[i], i);
                    
                    j++;
                }
            })
        })
    });
    async function load_images() {
        const image = await map.loadImage('https://maplibre.org/maplibre-gl-js/docs/assets/osgeo-logo.png');
        map.addImage('city-marker', image.data);
    }
    function draw_city(coords, n) {
        //UTTERLY DOGSHIT FUNCTION
        //Loads new geojson file for every city. Just hacky shit.
        map.addSource('cities-' + n.toString(), {
            'type': 'geojson',
            'data': {
                'type': 'FeatureCollection',
                'features': [
                    {
                        'type': 'Feature',
                        'geometry': {
                            'type': 'Point',
                            'coordinates': coords
                        }
                    }
                ]
            }
        });

        map.addLayer({
            'id': 'cities-'+n.toString(),
            'type': 'symbol',
            'source': 'cities-'+n.toString(),
            'layout': {
                'icon-image': 'city-marker',
            }
        });
    }
    function draw_circle(coords, radius, colour, n) {
        const options = {
            steps: 64,
            units: 'kilometers'
        };

        const circle = turf.circle(coords, radius, options);

        map.addSource('location-radius-'+n.toString(), {
            type: 'geojson',
            data: circle
        });

        map.addLayer({
            id: 'location-radius-'+n.toString(),
            type: 'fill',
            source: 'location-radius-'+n.toString(),
            paint: {
                'fill-color': colour,
                'fill-opacity': 0.5
            }
        });

        map.addLayer({
            id: 'location-radius-outline-'+n.toString(),
            type: 'line',
            source: 'location-radius-'+n.toString(),
            paint: {
                'line-color': colour,
                'line-width': 3
            }
        });
    }
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
            var p = i/n;
            var r = get_r(p).toString(16);
            var g = get_g(p).toString(16);
            var b = get_b(p).toString(16);
            if (r.length < 2)
                r = "0" + r;
            if (g.length < 2)
                g = "0" + g;
            if (b.length < 2)
                b = "0" + b;
            colArr.push("#" + r + g + b)
        }
        return colArr;
    }
})