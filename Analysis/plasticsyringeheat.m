function [T_history, Q_total] = analyzeSyringeHeat(params)
    % Time parameters
    dt = 0.001;
    t = 0:dt:params.duration;
    n_steps = length(t);

    % Initialize arrays
    T = zeros(1, n_steps);        % Air temperature
    T_wall = zeros(1, n_steps);   % Plastic wall temperature
    T_seal = zeros(1, n_steps);   % PTFE seal temperature
    V = zeros(1, n_steps);        % Volume
    Q_friction = zeros(1, n_steps); % Friction heat

    % Initial conditions
    T(1) = params.T_initial;
    T_wall(1) = params.T_initial;
    T_seal(1) = params.T_initial;
    V(1) = params.initial_volume;

    % Material properties
    % Air properties
    cp_air = 1005;        % J/kg·K
    rho_air = 1.225;      % kg/m³
    k_air = 0.024;        % W/m·K

    % Polypropylene properties (replacing glass)
    cp_plastic = 1920;    % J/kg·K
    rho_plastic = 946;    % kg/m³
    k_plastic = 0.12;     % W/m·K

    % PTFE properties (unchanged)
    cp_ptfe = 1.5;        % J/kg·K
    rho_ptfe = 2170;      % kg/m³
    k_ptfe = 0.292;       % W/m·K

    % Calculate thermal masses
    mass_plastic = rho_plastic * params.plastic_volume;
    mass_seal = rho_ptfe * params.seal_volume;

    % Natural convection coefficient (slightly lower for plastic)
    h_conv = 8;  % W/m²·K

    for i = 2:n_steps
        % Volume oscillation (12cc ± 1cc)
        V(i) = params.initial_volume + params.stroke_volume * sin(2*pi*params.frequency*t(i));
        
        % Calculate mass of air
        mass_air = rho_air * V(i);
        
        % Plunger velocity for friction calculation
        v = params.stroke_length * 2*pi*params.frequency * cos(2*pi*params.frequency*t(i));
        
        % Friction heat generation (W)
        Q_friction(i) = abs(v * params.normal_force * params.friction_coef);
        
        % Heat transfer between components
        % 1. Friction heat split between seal and air
        Q_seal = 0.7 * Q_friction(i);  % 70% to seal
        Q_air_friction = 0.3 * Q_friction(i);  % 30% to air
        
        % 2. Conductive heat transfer between components
        Q_seal_to_wall = k_ptfe * params.seal_contact_area * (T_seal(i-1) - T_wall(i-1)) / params.seal_thickness;
        
        % 3. Convective heat transfer from air to wall
        Q_air_to_wall = h_conv * params.internal_surface_area * (T(i-1) - T_wall(i-1));
        
        % 4. Convective heat loss to environment
        Q_wall_to_ambient = h_conv * params.external_surface_area * (T_wall(i-1) - params.T_ambient);
        
        % Temperature updates
        % Air temperature change
        dT_air = (Q_air_friction - Q_air_to_wall) * dt / (mass_air * cp_air);
        
        % Wall temperature change
        dT_wall = (Q_air_to_wall + Q_seal_to_wall - Q_wall_to_ambient) * dt / (mass_plastic * cp_plastic);
        
        % Seal temperature change
        dT_seal = (Q_seal - Q_seal_to_wall) * dt / (mass_seal * cp_ptfe);
        
        % Update temperatures
        T(i) = T(i-1) + dT_air;
        T_wall(i) = T_wall(i-1) + dT_wall;
        T_seal(i) = T_seal(i-1) + dT_seal;
    end

    % Temperature already in Celsius
    T_history = T;
    Q_total = sum(Q_friction * dt);
end

% Clear workspace and figures
clear;
close all;
clc;

% Setup parameters with measured geometry
params = struct();
params.initial_volume = 20e-6;        % 20 mL
params.stroke_volume = 1e-6;          % 1 mL stroke
params.stroke_length = 0.005;         % 5 mm stroke
params.frequency = 2;                 % 2 Hz
params.T_initial = 23;               % Initial temp °C
params.T_ambient = 23;               % Ambient temp °C
params.normal_force = 15;            % 15N compression force
params.friction_coef = 0.05;         % PTFE-plastic friction
params.duration = 250;               % 250 seconds simulation

% Measured geometry parameters (using same dimensions but renamed for clarity)
params.plastic_volume = 1.34456e-6;    % m³ plastic volume (same as previous glass volume)
params.seal_volume = 1.38918e-6;     % m³ PTFE seal volume (unchanged)
params.internal_surface_area = 4.8148e-3;  % m² internal surface area
params.external_surface_area = 5.51583e-3; % m² external surface area
params.seal_contact_area = 2.193e-4;  % m² seal contact area
params.seal_thickness = 0.006;        % m seal thickness

% Run simulation
[T_history, Q_total] = analyzeSyringeHeat(params);

% Create time vector for plotting
t = 0:params.duration/(length(T_history)-1):params.duration;

% Set up LaTeX interpreter for all text
set(0, 'defaultTextInterpreter', 'latex');
set(0, 'defaultAxesTickLabelInterpreter', 'latex');
set(0, 'defaultLegendInterpreter', 'latex');
set(0, 'defaultAxesFontSize', 12);

% Create figure with specified font settings
figure('Units', 'pixels', 'Position', [100, 100, 800, 500]);

% Plot with LaTeX formatting
plot(t, T_history, 'LineWidth', 1.5, 'DisplayName', 'Measured Temperature');
xlabel('Time (s)', 'Interpreter', 'latex');
ylabel('Temperature ($^{\circ}$C)', 'Interpreter', 'latex');
title('Air Temperature Inside Plastic Syringe', 'Interpreter', 'latex');
grid on;
set(gca, 'GridAlpha', 0.3);
set(gca, 'MinorGridAlpha', 0.15);
grid minor;

% Calculate axis limits
y_min = 22.9;
y_max = ceil(max(T_history));  % Round up to next 0.5
set(gca, 'YLim', [y_min y_max]);

% Calculate steady state metrics
steady_state_index = round(0.9 * length(T_history):length(T_history));
steady_state_temp = mean(T_history(steady_state_index));
steady_state_std = std(T_history(steady_state_index));

hold on;
yline(steady_state_temp, '--r', ['Steady State: ' num2str(steady_state_temp, '%.1f') ' $^{\circ}$C'], ...
    'LineWidth', 1, 'LabelHorizontalAlignment', 'left', 'Interpreter', 'latex', 'HandleVisibility', 'off');
yline(params.T_ambient, '--k', 'Ambient', 'LineWidth', 1, ...
    'LabelHorizontalAlignment', 'left', 'Interpreter', 'latex', 'HandleVisibility', 'off');

% Add legend with LaTeX formatting
legend('show', 'Location', 'southeast');

% Adjust axes properties
ax = gca;
ax.Box = 'on';
ax.TickLength = [0.02 0.02];
ax.LineWidth = 1;

% Display results with LaTeX-style symbols in command window
fprintf('\nSyringe Temperature Analysis Results:\n');
fprintf('Steady state temperature: %.2f ± %.2f °C\n', steady_state_temp, steady_state_std);
fprintf('Temperature rise above ambient: %.2f °C\n', steady_state_temp - params.T_ambient);
fprintf('Average heat generation rate: %.2f W\n', Q_total/params.duration);