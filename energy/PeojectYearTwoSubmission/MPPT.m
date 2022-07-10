%Plotting the different MPPT results
close all;
variable_step_mppt_power = readmatrix('ComparisonMPPT.xls', 'Range','A1:A2034');
small_step_mppt_power = readmatrix('ComparisonMPPT.xls', 'Range','G1:G2034');   %0.5% step
large_step_mppt_power = readmatrix('ComparisonMPPT.xls', 'Range','M1:M2034');   %1% step
medium_step_mppt_power = readmatrix('ComparisonMPPT.xls', 'Range','S1:S2034');   %0.7% step

plot(linspace(0,51.3,2034), variable_step_mppt_power, 'linewidth', 2, 'color', 'b');
hold on;
plot(linspace(0,51.3,2034), small_step_mppt_power, 'linewidth', 2, 'color', 'r');
hold on;
plot(linspace(0,51.3,2034), medium_step_mppt_power, 'linewidth', 2, 'color', 'g');
hold on;

xlim([0,51.3]);
legend('variable step', 'step size = 0.5%', 'step size = 0.7%');
xlabel('Time (s)');
ylabel('Power (mW)');
title('MPPT Algorithm performance');