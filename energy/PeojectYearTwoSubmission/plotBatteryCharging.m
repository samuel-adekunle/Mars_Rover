
close all;

SOC1 = readmatrix('BatCYCLEv20.xlsx', 'Range','G1:G12758');
SOC2 = readmatrix('BatCYCLEv20.xlsx', 'Range','H1:H12758');
SOC3 = readmatrix('BatCYCLEv20.xlsx', 'Range','I1:I12758');
state = readmatrix('BatCYCLEv20.xlsx', 'Range','A1:A12758');




plot(SOC1, '.b','MarkerSize',6);
hold on;
plot(SOC2, '.r','MarkerSize',4);
hold on;
plot(SOC3, '.g', 'MarkerSize',1);

%introduce shading
hold on;
charging = (state==1);
area(charging*200, 'LineStyle', 'none', 'FaceColor', 'g')
alpha(0.1);
hold on;
balancing = (state==4);
area(balancing*200, 'LineStyle', 'none','FaceColor', 'm')
alpha(0.1);
hold on;
lookUp = (state==3);
area(lookUp*200, 'LineStyle', '-', 'FaceColor', 'k')



legend('Cell 0', 'Cell 1', 'Cell 2', 'charging', 'balancing', 'SOC lookup');
xlabel('Time (s)');
ylabel('SOC (mAh)');
title('Charging');
ylim([0, 167]);
xlim([0,12758]);
