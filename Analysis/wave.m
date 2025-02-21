% Open and read the file
fid = fopen('csvdata.csv');
data = textscan(fid, '%s %f', 'Delimiter', ',');
fclose(fid);

% Extract time and pressure
timeStr = data{1};
pressure = data{2};

% Create time vector (in seconds from start)
time = (0:length(pressure)-1) * 0.1;  % Assuming ~0.1s between samples

% Ask user for duration to plot
duration = input('Enter the duration to plot (in seconds): ');

% Find the index corresponding to the requested duration
idx = find(time <= duration, 1, 'last');

% Create the plot using the specified duration
figure;
plot(time(1:idx), pressure(1:idx), 'b-', 'LineWidth', 1.5);
grid on;
xlabel('Time (seconds)');
ylabel('Pressure (PSI)');
title(['Pressure vs Time (First ' num2str(duration) ' seconds)']);

% Format the axes
ax = gca;
ax.FontSize = 12;
ax.GridAlpha = 0.3;

% Optional: Format y-axis limits for the selected data
ylim([min(pressure(1:idx))-0.1, max(pressure(1:idx))+0.1]);