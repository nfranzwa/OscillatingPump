// Complete project details: https://randomnerdtutorials.com/esp32-plot-readings-charts-multiple/

// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);

// Create Temperature Chart
var chartT = new Highcharts.Chart({
  chart:{
    renderTo:'chart-pressure'
  },
  series: [
    {
      name: 'Pressure #1',
      type: 'line',
      color: '#101D42',
    },

  ],
  title: {
    text: undefined
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'Pressure '
    }
  },
  credits: {
    enabled: false
  }
});


//Plot temperature in the temperature chart
function plotPressure(jsonValue) {

  var keys = Object.keys(jsonValue);
  var x = (new Date()).getTime();
  const key = keys[0];
  var y = Number(jsonValue[key]);

  if(chartT.series[i].data.length > 40) {
    chartT.series[i].addPoint([x, y], true, true, true);
  } else {
    chartT.series[i].addPoint([x, y], true, false, true);
  }

}


// Function to get current readings on the webpage when it loads for the first time
function getReadings(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotPressure(myObj);
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function(e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('message', function(e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_readings', function(e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotPressure(myObj);
  }, false);
}