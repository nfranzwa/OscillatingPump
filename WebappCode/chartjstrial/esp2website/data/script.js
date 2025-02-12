window.addEventListener('load', getReadings);
// Complete project details: https://randomnerdtutorials.com/esp32-web-server-websocket-sliders/

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getValues(){
    websocket.send("getValues");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
    getValues();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function updateSliderPWM(element) {
    var sliderNumber = element.id.charAt(element.id.length-1);
    var sliderValue = document.getElementById(element.id).value;
    document.getElementById("presslidervalue"+sliderNumber).innerHTML = sliderValue;
    console.log(sliderValue);
    websocket.send(sliderNumber+"s"+sliderValue.toString());
}

function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
        document.getElementById("slider"+ (i+1).toString()).value = myObj[key];
    }
}


/*
document.getElementById("pressureSlider").addEventListener("input", function () {
    document.getElementById("pressliderValue").textContent = this.value;
    updateslider();
});
*/

document.getElementById("OscillationSlider").addEventListener("input", function () {
    document.getElementById("oscillationsliderval").textContent = this.value;
});
document.getElementById("CuffVolumeSlider").addEventListener("input", function () {
    document.getElementById("cuffval").textContent = this.value;
});
document.getElementById("SustainTimeSlider").addEventListener("input", function () {
    document.getElementById("sustaintime").textContent = this.value;
});


const pressureData = [];

function updateSlider(element) {
    var sliderValue = document.getElementById("pressureSlider").value;
    document.getElementById("pressliderValue").innerHTML = sliderValue;
    console.log(sliderValue);
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/slider?value=" + sliderValue, true);
    xhr.send();
}
function updateChart(newValue) {
    pressureData.push(parseFloat(newValue)); // Ensure numerical values

    if (pressureData.length > 20) {
        pressureData.shift();
        pressureChart.data.labels.shift(); // Remove oldest label
    }

    // Update labels dynamically to match data points
    pressureChart.data.labels = pressureData.map((_, index) => index + 1);

    // Explicitly update the dataset reference
    pressureChart.data.datasets[0].data = [...pressureData];

    // Ensure Chart.js properly detects the update
    pressureChart.update('none');

}

const ctx = document.getElementById('pressuregraph').getContext('2d');
const pressureChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Pressure (cmH20)',
            data: [],
            borderColor: 'blue',
            borderWidth: 2,
            fill: false,
            tension: 0.2
        }]
    },
    options: {
        responsive: false,
        scales: {
            x: { title: { display: true, text: 'Data Points' } },
            y: { title: { display: true, text: 'Pressure (cmH20)' } }
        }
    }
});


function getReadings() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
        }
    };
    xhr.open("GET", "/readings", true);
    xhr.send();
}

if (!!window.EventSource) {
    var source = new EventSource('/events');

    source.addEventListener('open', function (e) {
        console.log("Events Connected");
    }, false);

    source.addEventListener('error', function (e) {
        if (e.target.readyState != EventSource.OPEN) {
            console.log("Events Disconnected");
        }
    }, false);

    source.addEventListener('pressure', function (e) {

        updateChart(e.data);
    }, false);
}
