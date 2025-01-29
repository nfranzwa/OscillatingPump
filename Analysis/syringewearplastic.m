function [results] = analyzeSealLife(params)
    % Material properties - PTFE seal
    ptfe_hardness = 30e6;     % Hardness (Pa)
    k_wear_base = 4.18e-4;    % Base wear coefficient (mm³/Nm) from paper
    min_seal_pressure = 25000; % Minimum sealing pressure (Pa)
    ptfe_spring_rate = 1.5e5; % N/m (adjusted for PP compliance)
    initial_interference = 0.1;% Initial seal interference fit (mm)
    
    % Time steps for analysis
    n_steps = 10000;          
    time_hours = linspace(0, params.duration_hours, n_steps);
    dt_hours = params.duration_hours/n_steps;
    
    % Initialize arrays
    normal_force = zeros(size(time_hours));
    wear_depth = zeros(size(time_hours));
    contact_pressure = zeros(size(time_hours));
    wear_rate = zeros(size(time_hours));
    
    % Initial conditions
    normal_force(1) = params.normal_force;
    contact_pressure(1) = normal_force(1)/params.seal_contact_area;
    wear_depth(1) = 0;
    
    % Operating conditions
    stroke_per_cycle = 2 * params.stroke_length; % m/cycle
    cycles_per_hour = params.frequency * 3600;   % cycles/hour
    sliding_distance_per_hour = stroke_per_cycle * cycles_per_hour; % m/hour
    
    % Main wear calculation loop
    for i = 2:n_steps
        % Calculate wear volume per hour (mm³/hour)
        wear_rate(i) = k_wear_base * normal_force(i-1) * sliding_distance_per_hour * 0.001;
        
        % Calculate wear depth increase for this time step (mm)
        depth_increase = wear_rate(i) * dt_hours / (params.seal_contact_area * 1e6);
        wear_depth(i) = wear_depth(i-1) + depth_increase;
        
        % Calculate remaining interference including PP compliance
        remaining_interference = max(0, initial_interference - wear_depth(i));
        
        % Calculate new normal force (reduced spring rate due to PP compliance)
        normal_force(i) = ptfe_spring_rate * (remaining_interference/1000);
        
        % Update contact pressure
        contact_pressure(i) = normal_force(i)/params.seal_contact_area;
        
        % Check for failure
        if contact_pressure(i) < min_seal_pressure
            fprintf('Seal failure predicted at %.1f hours\n', time_hours(i));
            fprintf('Seal failure predicted at %.1f days\n', time_hours(i)/24);
            fprintf('Seal failure predicted at %.1f months\n', time_hours(i)/24/30.44);
            fprintf('Final wear depth: %.3f mm\n', wear_depth(i));
            break;
        end
    end
    
    % Store results
    results = struct();
    results.time_hours = time_hours(1:i);
    results.normal_force = normal_force(1:i);
    results.wear_depth = wear_depth(1:i);
    results.contact_pressure = contact_pressure(1:i);
    results.wear_rate = wear_rate(1:i);
    
    % Display key results
    fprintf('\nKey Results:\n');
    fprintf('Initial wear rate: %.2e mm³/Nm\n', wear_rate(2)/(normal_force(2)*sliding_distance_per_hour));
    fprintf('Final wear rate: %.2e mm³/Nm\n', wear_rate(i)/(normal_force(i)*sliding_distance_per_hour));
    fprintf('Total wear depth: %.3f mm\n', wear_depth(i));
    fprintf('Final normal force: %.2f N\n', normal_force(i));
    fprintf('Final contact pressure: %.2f kPa\n', contact_pressure(i)/1000);
    
    % Plotting
    figure('Position', [100, 100, 1200, 400]);
    
    subplot(1,3,1)
    plot(time_hours(1:i)/24/30.44, normal_force(1:i), 'LineWidth', 1.5);
    xlabel('Time (months)');
    ylabel('Normal Force (N)');
    title('Seal Force vs Time');
    grid on;
    yline(min_seal_pressure*params.seal_contact_area, '--r', 'Minimum Required Force');
    
    subplot(1,3,2)
    plot(time_hours(1:i)/24/30.44, wear_depth(1:i), 'LineWidth', 1.5);
    xlabel('Time (months)');
    ylabel('Wear Depth (mm)');
    title('Wear Depth vs Time');
    grid on;
    
    subplot(1,3,3)
    plot(time_hours(1:i)/24/30.44, contact_pressure(1:i)/1000, 'LineWidth', 1.5);
    xlabel('Time (months)');
    ylabel('Contact Pressure (kPa)');
    title('Contact Pressure vs Time');
    grid on;
    yline(min_seal_pressure/1000, '--r', 'Minimum Sealing Pressure');
end

% Setup parameters
params = struct();
params.frequency = 2;                 % Hz
params.duration_hours = 50000;        % Extended to find failure
params.stroke_length = 0.005;         % m
params.normal_force = 15;             % N
params.seal_contact_area = 2.193e-4;  % m²

% Run analysis
results = analyzeSealLife(params);