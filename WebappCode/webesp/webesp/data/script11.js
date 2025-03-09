window.addEventListener('load', getReadings);
var calibrationstate = 0;
let hascalibrated = false;
function updateMinMaxValues() {
  const minInput = document.getElementById('MinPressureInput');
  const maxInput = document.getElementById('MaxPressureInput');

  // Get current values
  const minValue = parseInt(minInput.value);
  const maxValue = parseInt(maxInput.value);

  // Update min/max constraints
  if (minValue > maxValue) {
    minInput.value = maxValue;

    // Send updated value to server
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/MinPressure?value=" + maxValue, true);
    xhr.send();
  }
}

// Modify your existing changeValues function
function changeValues(button, step) {
  let input = button.parentElement.querySelector("input"); // Get the input field
  let name = input.getAttribute("name"); // Get the name attribute
  let maxLimit = Infinity;
  // Apply different max constraints based on input name

  if (name === "MinPressure") {
    const maxPressure = parseInt(document.getElementById('MaxPressureInput').value);
    maxLimit = maxPressure;
  }

  let newValue = parseInt(input.value) + step;
  newValue = Math.max(newValue, 0);
  if (maxLimit !== Infinity) {
    newValue = Math.min(newValue, maxLimit);
  }

  input.value = newValue;

  // Update constraints after value change
  updateMinMaxValues();

  // Send value to server
  var name2 = "/" + name + "?value=";
  var xhr = new XMLHttpRequest();
  xhr.open("GET", name2 + newValue, true);
  xhr.send();
}

// Initialize on page load
window.onload = function () {
  updateMinMaxValues();
};

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
let isRecording = false;
let isCalibrated = 0;
let hasrecorded = false;

// Function to update UI based on calibration state
function updateCalibrationUI() {
  const calibrationMessage = document.getElementById('calibration-message');
  
  // Handle button enabling/disabling
  if (calibrationstate == 0 || calibrationstate == 1) {
    document.getElementById('record-button').disabled = true;
    document.getElementById('standby-button').disabled = true;
  } else if (calibrationstate == 2 || calibrationstate == 3) {
    document.getElementById('record-button').disabled = false;
    document.getElementById('standby-button').disabled = false;
  }

  if (calibrationstate == 3) {
    document.getElementById('calibration-button').disabled = true;
  } else {
    document.getElementById('calibration-button').disabled = false;
  }

  // Update the calibration message
  if (calibrationstate == 1) {
    calibrationMessage.textContent = "Calibrating...";
    calibrationMessage.classList.remove("hidden");
    calibrationMessage.className = "calibration-status calibrating";
  } else if (calibrationstate == 2) {
    hascalibrated = true;
    calibrationMessage.textContent = "Calibration Done";
    calibrationMessage.classList.remove("hidden");
    calibrationMessage.className = "calibration-status complete";
  } else {
    calibrationMessage.classList.add("hidden");
  }
}

function toggleRecording(element) {
  hasrecorded = true;
  isRecording = !isRecording;
  const button = element;

  if (isRecording) {
    button.textContent = "Stop Recording";
    button.style.backgroundColor = "red";
    var path = "/record?value=1";
  } else {
    button.textContent = "Start Recording";
    button.style.backgroundColor = "#04AA6D";
    var path = "/record?value=0";
  }
  var xhr = new XMLHttpRequest();
  xhr.open("GET", path, true);
  xhr.send();
}

// Add event listener for toggle switch
document.getElementById("modeToggle").addEventListener("change", function () {
  const positionSlider = document.getElementById("position-slider");
  const sliderContainer = document.querySelector(".slider-container");

  if (this.checked) {
    // Manual mode
    calibrationstate = 3;
    var path = "/calibration?value=" + String(calibrationstate);
    positionSlider.disabled = false;
    sliderContainer.classList.remove("disabled");
  } else {
    if (hascalibrated == true) {
      calibrationstate = 2;
    } else {
      calibrationstate = 0;
    }
    // Automatic mode
    var path = "/calibration?value=" + String(calibrationstate);
    positionSlider.disabled = true;
    sliderContainer.classList.add("disabled");
  }

  // Update UI based on new calibration state
  updateCalibrationUI();

  var xhr = new XMLHttpRequest();
  xhr.open("GET", path, true);
  xhr.send();
});

function calibrating(button) {
  calibrationstate = 1;
  var path = "/calibration?value=" + String(calibrationstate);
  
  // Update UI based on new calibration state
  updateCalibrationUI();
  
  var xhr = new XMLHttpRequest;
  xhr.open("GET", path, true);
  xhr.send();
}

function standby(element) {
  calibrationstate = 4;
  hascalibrated = false;
  var path = "/calibration?value=" + String(calibrationstate);
  
  // Update UI based on new calibration state
  updateCalibrationUI();
  
  var xhr = new XMLHttpRequest;
  xhr.open("GET", path, true);
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

  // Initialize the slider state based on the default toggle state
  const modeToggle = document.getElementById("modeToggle");
  const positionSlider = document.getElementById("position-slider");
  const sliderContainer = document.querySelector(".slider-container");

  if (!modeToggle.checked) {
    // If automatic mode is the default (toggle not checked)
    positionSlider.disabled = true;
    sliderContainer.classList.add("disabled");
  }
  
  // Initialize UI based on starting calibration state
  updateCalibrationUI();
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
    // Update the calibration state
    let newcalibrationstate = parseInt(e.data);
    if (newcalibrationstate == 2) {
      calibrationstate = 2;
      // Update UI based on new calibration state from server
      updateCalibrationUI();
    }
  }, false);
}