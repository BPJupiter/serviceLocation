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
        if (!validate_ip(ip)) {
            $('#search').val('');
            $('#search').attr("placeholder", "Invalid IPv4 address.")
            return;
        }

        $('#routeText').attr('style', 'animation: routing 0.7s infinite alternate');

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
            let prev = pingJson.latencies[0]
            let current = pingJson.latencies[1]
            relLatencies.push(current);
            for (let i = 1; i < nHops; i++) {
                current = pingJson.latencies[i];
                if (current == -1) {
                    relLatencies.push(0);
                    continue;
                }
                var latency = current - prev;
                relLatencies.push(Math.abs(latency));
                prev = current;
            }
            console.log(relLatencies)
            fetch('http://localhost:5000/locate?ipArr='+pingJson.addresses)
            .then((response) => {
                return response.json();
            })
            .then((coordJson) => {
                console.log(coordJson.coords)
                
                var cities_geojson = {
                    'type': 'geojson',
                    'data': {
                        'type': 'FeatureCollection',
                        'features': []
                    }
                };

                var lines_geojson = {
                    'type': 'geojson',
                    'data': {
                        'type': 'Feature',
                        'properties': {},
                        'geometry': {
                            'type': 'LineString',
                            'coordinates': []
                        }
                    }
                }

                let j = 0 //index to colourArr
                var curr_coords = coordJson.coords[1];
                var prev_coords = coordJson.coords[0];
                var city_drawn = false;
                add_line(lines_geojson, prev_coords);
                for (let i = 1; i < nHops; i++) {
                    if (coordJson.coords[i][0] != null) {
                        curr_coords = coordJson.coords[i];
                        add_city(curr_coords, pingJson.latencies[i], cities_geojson);
                        city_drawn = true;
                        add_line(lines_geojson, curr_coords);
                    }
                    
                    if (relLatencies[i] == 0) {
                        j++
                        continue
                    }
                    var radius = relLatencies[i] * 100;
                    draw_circle(prev_coords, radius, colourArr[j], i, city_drawn);

                    if (city_drawn) {
                        prev_coords = curr_coords;
                    }
                    
                    j++;
                }
                draw_cities(cities_geojson);
                draw_lines(lines_geojson);
                $('#routeText').attr('style', 'animation: idle');
                $('#completeText').attr('style', 'animation: complete 3s')
            })
        })
    });

    $('#reset').click(function() {
        location.reload();
    });
    
    async function load_images() {
        const image = await map.loadImage('https://maplibre.org/maplibre-gl-js/docs/assets/osgeo-logo.png');
        map.addImage('city-marker', image.data);
    }

    function add_city(coords, latency, cities_geojson) {
        cities_geojson['data']['features'].push({
            'type': 'Feature',
            'geometry': {
                'type': 'Point',
                'coordinates': coords
            },
            'properties': {'ping': latency.toString()}
        });
    }

    function draw_cities(cities_geojson) {
        //UTTERLY DOGSHIT FUNCTION
        //Loads new geojson file for every city. Just hacky shit.
        map.addSource('cities', cities_geojson);

        map.addLayer({
            'id': 'cities',
            'type': 'symbol',
            'source': 'cities',
            'layout': {
                'icon-image': 'city-marker',
                'text-field': ['get', 'ping'],
                'text-font': ['Noto Sans Regular'],
                'text-offset': [0,1.25],
                'text-anchor': 'top'
            },
            'paint': {
                'text-color': '#FFFFFF'
            }
        });
    }

    function draw_circle(coords, radius, colour, n, known_position) {
        var opacity = 0.3;
        if (known_position) {
            opacity = 0.1;
        }
        if (coords[0] == null) {
            return;
        }

        const options = {
            steps: 64,
            units: 'kilometers'
        };

        const circle = turf.circle(coords, radius, options);
        let max_min_lat = get_max_min_lat(circle.geometry.coordinates[0]);
        let rects = get_num_rects(circle.geometry.coordinates[0]);
        if (rects[0] == 1 || rects[1] == 1) {
            draw_rect(max_min_lat, rects, colour, n, known_position);
        }

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
                'fill-opacity': opacity
            }
        });
        /*
        map.addLayer({
            id: 'location-radius-outline-'+n.toString(),
            type: 'line',
            source: 'location-radius-'+n.toString(),
            paint: {
                'line-color': colour,
                'line-width': 3
            }
        });*/
        function get_max_min_lat(coordArr) {
            latArr = new Array();
            for (let i=0; i < coordArr.length; i++) {latArr.push(coordArr[i][1]);}
            return [Math.max.apply(null, latArr), Math.min.apply(null, latArr)]
        }
        function get_max_min_long(coordArr) {
            longArr = new Array();
            for (let i=0; i < coordArr.length; i++) {longArr.push(coordArr[i][0]);}
            return [Math.max.apply(null, longArr), Math.min.apply(null, longArr)]
        }
        function get_num_rects(coordArr) {
            let jumps = [0,0]; // top, bottom
            let n = 0;
            //console.log(coordArr);
            for (let i=1; i < coordArr.length; i++) {
                if (Math.abs(coordArr[i][0] - coordArr[i-1][0]) > 240) {
                    if (coordArr[i][1] > 0) {
                        n++;
                        jumps[0] = 1;
                    }
                    else {
                        n++;
                        jumps[1] = 1;
                    }
                }
            }
            if (n==2) {
                jumps = [1, 1];
            }
            return jumps;
        }
    }

    function draw_rect(max_min_lat, rects, colour, n, known_position) {
        var opacity = 0.3;
        if (known_position) {
            opacity = 0.1;
        }
        const topRect = turf.polygon([
            [
                [0, max_min_lat[0]],
                [360, max_min_lat[0]],
                [360, 90],
                [0, 90],
                [0, max_min_lat[0]]
            ],
        ]);
        const bottomRect = turf.polygon([
            [
                [0, max_min_lat[1]],
                [360, max_min_lat[1]],
                [360, -90],
                [0, -90],
                [0, max_min_lat[1]]
            ]
        ]);

        if (rects[0] == 1) {
            map.addSource('toprect-'+n.toString(), {
                type: 'geojson',
                data: topRect
            });

            map.addLayer({
                id: 'toprect-'+n.toString(),
                type: 'fill',
                source: 'toprect-'+n.toString(),
                paint: {
                    'fill-color': colour,
                    'fill-opacity': opacity
                }
            });
        }
        if (rects[1] == 1) {
            map.addSource('bottomrect-'+n.toString(), {
                type: 'geojson',
                data: bottomRect
            });

            map.addLayer({
                id: 'bottomrect-'+n.toString(),
                type: 'fill',
                source: 'bottomrect-'+n.toString(),
                paint: {
                    'fill-color': colour,
                    'fill-opacity': opacity
                }
            });
        }
    }

    function add_line(lines_geojson, coords) {
        if (coords[0] < 0) {
            coords[0] += 360;
        }
        lines_geojson['data']['geometry']['coordinates'].push(coords);
    }

    function draw_lines(lines_geojson) {
        map.addSource('lines',  lines_geojson);
        map.addLayer({
            'id': 'lines',
            'type': 'line',
            'source': 'lines',
            'layout': {
                'line-join': 'round',
                'line-cap': 'round'
            },
            'paint': {
                'line-color': '#00FF00',
                'line-width': 4
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

    function validate_ip(ip) {
        const ipv4 = 
            /^(\d{1,3}\.){3}\d{1,3}$/;
        const ipv6 = 
            /^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$/;
        return ipv4.test(ip);// || ipv6.test(ip);
    }
    
    function test_circle() {
        map.on('load', () => {
            const options = {
                steps: 64,
                units: 'kilometers'
            };
            const foo = turf.circle([-122.33, 47.6], 80 * 75, options);
            var coordArr = foo.geometry.coordinates[0];
            var latArr = new Array();
            for (let i=0; i < coordArr.length; i++) {latArr.push(coordArr[i][1]);}
            var max = Math.max.apply(null, latArr);
            console.log(max);
            latArr.splice(latArr.indexOf(max), 1);
            var second = Math.max.apply(null, latArr);
            console.log(second);
            console.log(foo.geometry.coordinates);
            const fee = turf.polygon([
                [
                    [0, second],
                    [360, second],
                    [360, 90],
                    [0, 90],
                    [0, second]
                ],
            ]);

            map.addSource('fee', {
                type: 'geojson',
                data: fee
            });

            map.addLayer({
                id: 'fee',
                type: 'fill',
                source: 'fee',
                paint: {
                    'fill-color': '#FFFFFF',
                    'fill-opacity': 0.3
                }
            });

            map.addSource('foo', {
                type: 'geojson',
                data: foo
            });

            map.addLayer({
                id: 'foo',
                type: 'fill',
                source: 'foo',
                paint: {
                    'fill-color': '#FFFFFF',
                    'fill-opacity': 0.3
                }
            });
        });
    }
})