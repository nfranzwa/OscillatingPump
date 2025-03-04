window.addEventListener('load', getReadings);

function changeValues(button, step) {

  let input = button.parentElement.querySelector("input"); // Get the input field inside the same .input-group
  let name = input.getAttribute("name"); // Get the name attribute of the input
  let newValue = Math.min(Math.max(parseInt(input.value) + step, 0), 30); // Ensure value stays in range
  input.value = newValue;
  var name2 = "/" + name + "?value=";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", name2 + newValue, true);
  xhr.send();

}
document.getElementById("position-slider").addEventListener("input", function () {
  let sliderValue = this.value; // Get the current slider value
  editslider(sliderValue); // Send it via XMLHttpRequest
});
function editslider(value) {
  var xhr = new XMLHttpRequest();
  let url = "/position?value=" + value;
  xhr.open("GET", url, true);
  xhr.send();
}
const pressureData = [];
let calibrationstate = 0;
let isRecording = false;
let isCalibrated = 0;
let hasrecorded = false;
//const recordMsg = document.getElementById("record-message");
function toggleRecording(element) {
  hasrecorded = true;
  isRecording = !isRecording;
  const button = element;
  // const recordMsg = document.getElementById("record-message");

  if (isRecording) {
    button.textContent = "Stop Recording";
    button.style.backgroundColor = "red";
    var path = "/record?value=1";
    // Hide the message while recording
    //recordMsg.textContent = "";
    //recordMsg.classList.add("hidden");
  } else {
    button.textContent = "Start Recording";
    button.style.backgroundColor = "#04AA6D";
    var path = "/record?value=0";
    //recordMsg.textContent = "Data recorded. Can be found in folder titled data (pinned on shelf)";
    //recordMsg.classList.remove("hidden");
  }
  var xhr = new XMLHttpRequest();
  xhr.open("GET", path, true);
  xhr.send();
}

document.getElementById("modeToggle").addEventListener("change", function () {
  if (this.checked) {
    var path = "/calibration?value=3";

  } else {
    var path = "/calibration?value=0";

  }
  var xhr = new XMLHttpRequest();
  xhr.open("GET", path, true);
  xhr.send();
});

function calibrating(button) {
  var path = "/calibration?value=1";
  var xhr = new XMLHttpRequest;
  xhr.open("GET", path, true);
  xhr.send();
}

function standby(element) {

}



function updatecalibr(newValue) {
  calibrationstate = newValue;
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

  source.addEventListener('calstate', function (e) {
    calibrationstate = e.data;
    calstr = String(calibrationstate);
    var path = "/calibration?value=" + calstr;
    var xhr = new XMLHttpRequest;
    xhr.open("GET", path, true);
    xhr.send();

    const calibrationMessage = document.getElementById('calibration-message'); F
    if (calibrationstate == 0 || calibrationstate == 1 || calibrationstate == 3) {
      document.getElementById('record-button').disabled = true;
      document.getElementById('standby-button').disabled = true;
    } else if (calibrationstate == 2) {

      document.getElementById('record-button').disabled = false;
      document.getElementById('standby-button').disabled = false;
    }
    if (calibrationstate == 3) {
      document.getElementById('calibration-button').disabled = true;
    } else {
      document.getElementById('calibration-button').disabled = false;
    }

    if (calibrationstate == 1) {
      calibrationMessage.textContent = "Calibrating! Please Wait";
      calibrationMessage.classList.remove("hidden");
    } else {
      calibrationMessage.classList.add("hidden");
    }
  });
} 