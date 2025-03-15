window.addEventListener('load', getReadings);
var calibrationstate = 0;
let hascalibrated = false;

// Add a status message area to provide feedback
function addStatusMessage(message, isError = false) {
  const statusArea = document.getElementById('status-message') || createStatusArea();
  statusArea.textContent = message;
  statusArea.className = isError ? 'status-message error' : 'status-message success';
  
  // Clear message after 3 seconds
  setTimeout(() => {
    statusArea.textContent = '';
    statusArea.className = 'status-message';
  }, 3000);
}

// Create status area if it doesn't exist
function createStatusArea() {
  const statusArea = document.createElement('div');
  statusArea.id = 'status-message';
  statusArea.className = 'status-message';
  document.querySelector('.container').after(statusArea);
  return statusArea;
}

function updateMinMaxValues() {
  const minInput = document.getElementById('MinPressureInput');
  const maxInput = document.getElementById('MaxPressureInput');
  const minValue = parseInt(minInput.value);
  const maxValue = parseInt(maxInput.value);
  
  if (minValue > maxValue) {
    minInput.value = maxValue;
    sendValueToServer("MinPressure", maxValue);
  }
  updateChartScale();
}

// Improved function to send values to server with feedback
function sendValueToServer(name, value) {
  const path = "/" + name + "?value=" + value;
  var xhr = new XMLHttpRequest();
  
  xhr.onreadystatechange = function() {
    if (this.readyState == 4) {
      if (this.status == 200) {
        console.log("Value sent successfully: " + name + " = " + value);
        addStatusMessage(name + " updated to " + value);
      } else {
        console.error("Error sending value: " + name);
        addStatusMessage("Failed to update " + name, true);
      }
    }
  };
  
  xhr.onerror = function() {
    console.error("Request failed");
    addStatusMessage("Connection error", true);
  };
  
  xhr.open("GET", path, true);
  xhr.timeout = 5000; // 5 second timeout
  xhr.send();
}

function changeValues(button, step) {
  let input = button.parentElement.querySelector("input");
  let name = input.getAttribute("name");
  let maxLimit = Infinity;

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

  // Send value to server with improved function
  sendValueToServer(name, newValue);
  
  // Visual feedback that value is being sent
  button.disabled = true;
  setTimeout(() => {
    button.disabled = false;
  }, 300);

  updateChartScale();
}

// Initialize on page load
window.onload = function () {
  updateMinMaxValues();
  updateChartScale();
  
  // Add input event listeners for direct input
  document.querySelectorAll('input[type="number"]').forEach(input => {
    input.addEventListener('change', function() {
      const name = this.getAttribute('name');
      const value = this.value;
      sendValueToServer(name, value);
    });
  });
};

const pressureData = [];
let isRecording = false;
let isCalibrated = 0;
let hasrecorded = false;

function updateCalibrationUI() {
  const calibrationMessage = document.getElementById('calibration-message');
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
  if (calibrationstate == 1) {
    calibrationMessage.textContent = "Calibrating...";
    calibrationMessage.classList.remove("hidden");
    calibrationMessage.className = "calibration-status calibrating";
  } else if (calibrationstate == 2) {
    hascalibrated = true;
    calibrationMessage.textContent = "Calibration Done";
    calibrationMessage.classList.remove("hidden");
    calibrationMessage.className = "calibration-status complete";
  } else if (calibrationstate == 4) {
    calibrationMessage.textContent = "Standby Mode";
    calibrationMessage.classList.remove("hidden");
    calibrationMessage.className = "calibration-status standby";
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
    var value = 1;
  } else {
    button.textContent = "Start Recording";
    button.style.backgroundColor = "#04AA6D";
    var value = 0;
  }
  
  sendValueToServer("record", value);
}

function calibrating(button) {
  calibrationstate = 1;
  button.disabled = true;
  setTimeout(() => {
    button.disabled = false;
  }, 1000);
  
  updateCalibrationUI();
  sendValueToServer("calibration", calibrationstate);
}

function standby(element) {
  calibrationstate = 4;
  hascalibrated = false;
  element.disabled = true;
  setTimeout(() => {
    element.disabled = false;
  }, 1000);
  
  updateCalibrationUI();
  sendValueToServer("calibration", calibrationstate);
}

function updateChart(newValue) {
  pressureData.push(parseFloat(newValue));
  if (pressureData.length > 20) {
    pressureData.shift();
    pressureChart.data.labels.shift();
  }
  pressureChart.data.labels = pressureData.map((_, index) => index + 1);
  pressureChart.data.datasets[0].data = [...pressureData];
  pressureChart.update('none');
}

// Change the chart options to use fixed scale values
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
      y: {
        title: { display: true, text: 'Pressure (cmH20)' },
        min: -30,  // Fixed minimum value
        max: 90    // Fixed maximum value
      }
    }
  }
});

// Update the updateChartScale function to not change the scale
function updateChartScale() {
  // No need to adjust the scale anymore since we're using fixed values
  pressureChart.update();
}

function getReadings() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      console.log("Got readings from server");
      // Parse response if needed
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
  updateCalibrationUI();
}

// Add extra CSS to the page for status messages
function addStyles() {
  const style = document.createElement('style');
  style.textContent = `
    .status-message {
      text-align: center;
      padding: 8px;
      margin: 10px 0;
      border-radius: 4px;
      transition: opacity 0.5s;
      height: 20px;
    }
    .status-message.success {
      background-color: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .status-message.error {
      background-color: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
  `;
  document.head.appendChild(style);
}

// Run once on page load
addStyles();

// Setup event source for server sent events
if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function (e) {
    console.log("Events Connected");
    addStatusMessage("Connected to server");
  }, false);

  source.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
      addStatusMessage("Disconnected from server", true);
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

  source.addEventListener('errorcode', function (e) {
    let errorcode = parseInt(e.data);
    const calibrationMessage = document.getElementById('calibration-message');
    if (errorcode == 1) {
      calibrationMessage.textContent = "Calibration Fail";
      calibrationMessage.classList.remove("hidden");
      calibrationMessage.className = "calibration-status error";
      addStatusMessage("Error detected", true);
    } else if (errorcode == 2){
      calibrationMessage.textContent = "Automatic Fail (Offset Out of Bounds)";
      calibrationMessage.classList.remove("hidden");
      calibrationMessage.className = "calibration-status error";
      addStatusMessage("Error detected", true);
    }
  }, false);
}