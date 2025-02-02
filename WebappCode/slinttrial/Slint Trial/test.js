import { BarChart } from 'chartist';

new BarChart('#chart', {
  labels: ['W1', 'W2', 'W3', 'W4', 'W5', 'W6', 'W7', 'W8', 'W9', 'W10'],
  series: [
    [1, 2, 4, 8, 6, -2, -1, -4, -6, -2]
  ]
}, {
  high: 10,
  low: -10,
  axisX: {
    labelInterpolationFnc: (value, index) => (index % 2 === 0 ? value : null)
  }
});