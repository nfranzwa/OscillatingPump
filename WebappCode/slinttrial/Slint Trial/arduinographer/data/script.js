window.addEventListener('load',getReadings)

const pressureData = [];

function updateChart(newValue) {
    pressureData.push(newValue);
    if (pressureData.length > 20) {
        pressureData.shift();
    }
    pressureChart.data.datasets[0].data = pressureData;
    pressureChart.update();
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
        responsive: true,
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
