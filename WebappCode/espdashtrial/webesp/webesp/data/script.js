window.addEventListener('load', getReadings);


function updateslider(element) {
  var sliderNumber = element.id.charAt(element.id.length - 1);
  var sliderValue = document.getElementById(element.id).value;
  //var sliderValue = document.getElementById("pressureSlider").value;
  document.getElementById("pressliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/pressureslider?value=" + sliderValue, true);
  xhr.send();
}
window.updateslider = updateslider;


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