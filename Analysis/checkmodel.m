clear all;
close all;
clc;

% Constants
R = 8.314;        % Gas constant (J/mol·K)
T = 298.15;       % Temperature (K)
a = 0.13476225;   % van der Waals constant (Pa·m⁶/mol²)
b = 0.0366e-3;    % van der Waals constant (m³/mol)
atm = 101325;     % Atmospheric pressure (Pa)
r = 0.011;        % Variable stent radius (m)          CHANGE 
h = 0.0255;       % Height of cuff (m)

% Get target pressure from user
target_psi = input('Enter target pressure (psi): ');
target_Pa = (target_psi + 14.7) * 6894.76; % Convert psig to Pa

% Container volume (fixed)
V_container = pi*r^2*h;

% Function to calculate pressure difference from target
function pressure_diff = pressure_error(V_air, target_Pa, constants)
    % Unpack constants
    R = constants.R;
    T = constants.T;
    a = constants.a;
    b = constants.b;
    atm = constants.atm;
    V_container = constants.V_container;
    
    % Calculate number of moles using initial conditions
    n = (atm * V_air)/(R * T);
    
    % Calculate pressure using van der Waals equation
    term1 = (a * n^2)/(V_container^2);
    term2 = (V_container - n*b);
    term3 = n * R * T;
    
    P_final = (term3 - term1 * term2)/term2;
    
    % Return difference from target pressure
    pressure_diff = P_final - target_Pa;
end

% Package constants for the solver
constants.R = R;
constants.T = T;
constants.a = a;
constants.b = b;
constants.atm = atm;
constants.V_container = V_container;

% Initial guess for V_air (using ideal gas approximation)
V_air_guess = target_Pa * V_container / atm;

% Find required V_air using fzero
options = optimset('Display', 'off');
V_air = fzero(@(v) pressure_error(v, target_Pa, constants), V_air_guess, options);

% Calculate final pressure to verify
n = (atm * V_air)/(R * T);
term1 = (a * n^2)/(V_container^2);
term2 = (V_container - n*b);
term3 = n * R * T;
P_final = (term3 - term1 * term2)/term2;

% Display results
fprintf('\nResults:\n');
fprintf('Required air volume: %.3e m³ (%.2f µL)\n', V_air, V_air*1e9);
fprintf('Verification pressure: %.2f psi\n', (P_final/6894.76) - 14.7);