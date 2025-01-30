% Clear workspace and figures
clear;
close all;
clc;

% Material Properties
% PTFE Seal
E_ptfe = 600e6;           % Young's modulus (Pa)
v_ptfe = 0.46;           % Poisson's ratio
rho_ptfe = 2170;         % Density (kg/m^3)
comp_set = 0.00;         % Compression set (0%)
mu = 0.05;               % Friction coefficient
hardness_ptfe = 58.2;    % Shore D hardness

% Borosilicate Glass
E_glass = 62.5e9;        % Young's modulus (Pa)
v_glass = 0.2;           % Poisson's ratio
rho_glass = 2250;        % Density (kg/m^3)

% Geometry
d_seal_original = 19.4e-3;    % Original seal diameter (m)
d_seal_compressed = 1.935e-2;  % Compressed seal diameter (m)
interference = 5e-5;        % Radial interference (m)
rib_thickness = 1.2e-3;       % Rib thickness (m)
rib_height = 1.0e-3;         % Rib height (m)
rib_spacing = 1.2e-3;        % Spacing between ribs (m)
seal_height = 6e-3;          % Total seal height (m)
barrel_thickness = 0.79e-3;   % Barrel wall thickness (m)

% Operating Conditions
freq = 1.67;             % Frequency (Hz)
stroke = 2e-3;           % Stroke length (m)
p_max = (0.5 + 14.7) * 6894.76;   % Max pressure (Pa) (converting from psi)
temp = 23 + 273.15;      % Temperature (K)
duration = 24 * 24 * 3600; % Duration in seconds (24 days)

% Break-in period parameters
t_breakin = 5*24*3600;   % 2 day break-in period
K_initial = 40e-18;      % Initial wear rate (break-in period)
K_steady = 5e-18;        % Steady-state wear rate

% Calculate total cycles
total_cycles = freq * duration;

% Calculate effective elastic modulus (plane strain)
E_eff = 1 / ((1-v_ptfe^2)/E_ptfe + (1-v_glass^2)/E_glass);

% Calculate contact pressure (Lamé solution for interference fit)
r_mean = d_seal_compressed/2;
p_contact = interference * E_eff / r_mean;

% Calculate friction force per rib
num_ribs = 3;
contact_area_per_rib = pi * d_seal_compressed * rib_thickness;
friction_force_per_rib = p_contact * contact_area_per_rib * mu;
total_friction_force = friction_force_per_rib * num_ribs;

% Time points for analysis
t = linspace(0, duration, 1000);
dt = t(2) - t(1);  % Time step

% Calculate time-dependent wear rate
wear_rate_t = K_initial * exp(-t/t_breakin) + K_steady * (1 - exp(-t/t_breakin));

% Calculate normal force
normal_force_per_rib = p_contact * contact_area_per_rib;

% Calculate sliding distance per time step
sliding_distance_per_step = 2 * stroke * freq * dt;

% Calculate wear depth (corrected calculation)
wear_increment = wear_rate_t .* normal_force_per_rib * sliding_distance_per_step;
wear_depth_t = cumsum(wear_increment);  % Accumulated wear depth

% Calculate compression set effect over time
comp_set_factor = 1 - (1 - comp_set) * exp(-t/t_breakin);
comp_set_loss = interference * comp_set_factor;

% Calculate total interference loss (wear + compression set)
total_interference_loss = wear_depth_t + comp_set_loss;

% Calculate remaining interference
remaining_interference = max(0, interference - total_interference_loss);

% Calculate material loss based on interference loss
material_loss = interference - remaining_interference;

% Calculate sealing pressure over time
seal_pressure = E_eff * remaining_interference / r_mean;

% Set up LaTeX interpreter for all text
set(0, 'defaultTextInterpreter', 'latex');
set(0, 'defaultAxesTickLabelInterpreter', 'latex');
set(0, 'defaultLegendInterpreter', 'latex');
set(0, 'defaultAxesFontSize', 12);

% Create figure with specified font settings
figure('Units', 'pixels', 'Position', [100, 100, 800, 800]);

% Subplot 1: Pressure over time
subplot(2,1,1);
plot(t/3600/24, seal_pressure/1e6, 'b-', 'LineWidth', 2, 'DisplayName', 'Contact Pressure');
hold on;
plot([0 duration/3600/24], [p_max p_max]/1e6, 'r--', 'LineWidth', 1.5, 'DisplayName', 'Required Pressure');
xlabel('Time (days)', 'Interpreter', 'latex');
ylabel('Pressure (MPa)', 'Interpreter', 'latex');
title('Glass Seal Contact Pressure vs Time', 'Interpreter', 'latex');
legend('Location', 'northeast');
grid on;
ylim([0 max(seal_pressure/1e6)*1.1]);
xlim([0 24]);
set(gca, 'GridAlpha', 0.3);
set(gca, 'MinorGridAlpha', 0.15);
grid minor;

% Find intersection point for annotation
failure_idx = find(seal_pressure < p_max, 1);
if ~isempty(failure_idx)
    failure_time = t(failure_idx)/3600/24;
    failure_pressure = seal_pressure(failure_idx)/1e6;
    plot(failure_time, failure_pressure, 'ko', 'MarkerFaceColor', 'k', 'HandleVisibility', 'off')
    % Move text above the point
    text(failure_time + 0.5, failure_pressure * 2.2, sprintf('Failure at %.1f days', failure_time), ...
        'Interpreter', 'latex');
end

% Subplot 2: Material loss over time
subplot(2,1,2);
plot(t/3600/24, material_loss*1e6, 'r-', 'LineWidth', 2, 'DisplayName', 'Material Loss');
hold on;
plot([0 duration/3600/24], [interference*1e6 interference*1e6], 'k--', 'LineWidth', 1.5, ...
    'DisplayName', 'Max Loss ($50\,\mu$m)');
xlabel('Time (days)', 'Interpreter', 'latex');
ylabel('Material Loss ($\mu$m)', 'Interpreter', 'latex');
title('Material Loss vs Time', 'Interpreter', 'latex');
legend('Location', 'northwest');
grid on;
ylim([0 interference*1e6*1.1]); % Show up to 110% of interference
xlim([0 24]);
set(gca, 'GridAlpha', 0.3);
set(gca, 'MinorGridAlpha', 0.15);
grid minor;

% Add annotation for material loss at failure point
if ~isempty(failure_idx)
    loss_at_failure = material_loss(failure_idx)*1e6;
    plot(failure_time, loss_at_failure, 'ko', 'MarkerFaceColor', 'k', 'HandleVisibility', 'off')
    % Move text below the point
    text(failure_time + 0.5, loss_at_failure * 0.95, ...
        sprintf('Loss at failure: %.1f $\\mu$m', loss_at_failure), ...
        'Interpreter', 'latex');
end

% Calculate time to failure
if isempty(failure_idx)
    time_to_failure = duration;  % No failure occurred
else
    time_to_failure = t(failure_idx);  % Time at which failure occurred
end

% Display key results
fprintf('\nSyringe Seal Analysis Results:\n');
fprintf('Initial contact pressure: %.2f MPa\n', p_contact/1e6);
fprintf('Initial interference: %.2f μm\n', interference*1e6);
fprintf('Time to failure: %.1f days\n', time_to_failure/3600/24);
if ~isempty(failure_idx)
    fprintf('Material loss at failure: %.2f μm\n', loss_at_failure);
end
fprintf('Safety factor against leakage at start: %.2f\n', (seal_pressure(1)/p_max));
fprintf('Average daily material loss: %.2f μm/day\n', (material_loss(end)*1e6)/24);