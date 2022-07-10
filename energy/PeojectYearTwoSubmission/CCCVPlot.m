
%plot CC/CV
close all;

SOC1 = readmatrix('CCCV_BatCYCLEv21.xlsx', 'Range','G600:G1700');
SOC2 = readmatrix('CCCV_BatCYCLEv21.xlsx', 'Range','H600:H1700');
SOC3 = readmatrix('CCCV_BatCYCLEv21.xlsx', 'Range','I600:I1700');
state = readmatrix('CCCV_BatCYCLEv21.xlsx', 'Range','A600:A1700');

current = readmatrix('CCCV_BatCYCLEv21.xlsx', 'Range','D600:D1700');


subplot(2, 1, 1);
plot(SOC1, '.b','MarkerSize',6);
hold on;
plot(SOC2, '.r','MarkerSize',4);
hold on;
plot(SOC3, '.g', 'MarkerSize',1);

%introduce shading
hold on;
charging = (state==1);
area(charging*1000, 'LineStyle', 'none', 'FaceColor', 'g')
alpha(0.1);
hold on;
balancing = (state==4);
area(balancing*1000, 'LineStyle', 'none','FaceColor', 'm')
alpha(0.1);
hold on;
lookUp = (state==3);
area(lookUp*1000, 'LineStyle', '-', 'FaceColor', 'k')
alpha(0.1);
hold on;
CV = (state==5);
area(CV*1000, 'LineStyle', '-', 'FaceColor', [1,0.9,0.2])
alpha(0.1);
Ready = (state==6);
area(Ready*1000, 'LineStyle', '-', 'FaceColor', 'b')
alpha(0.1);



legend('Cell 0', 'Cell 1', 'Cell 2', 'charging', 'balancing', 'SOC lookup', 'CV charging', 'Ready');
xlabel('Time (s)');
ylabel('SOC (mAh)');
title('CC/CV SOC');
ylim([320, 600]);
xlim([0,1100]);

subplot(2, 1, 2);
plot(current, '.k','MarkerSize',6);
hold on;

%introduce shading
hold on;
charging = (state==1);
area(charging*1000, 'LineStyle', 'none', 'FaceColor', 'g')
alpha(0.1);
hold on;
balancing = (state==4);
area(balancing*1000, 'LineStyle', 'none','FaceColor', 'm')
alpha(0.1);
hold on;
lookUp = (state==3);
area(lookUp*1000, 'LineStyle', '-', 'FaceColor', 'k')
alpha(0.1);
hold on;
CV = (state==5);
area(CV*1000, 'LineStyle', '-', 'FaceColor', [1,0.9,0.2])
alpha(0.1);
Ready = (state==6);
area(Ready*1000, 'LineStyle', '-', 'FaceColor', 'b')
alpha(0.1);


legend('Charging current', 'charging', 'balancing', 'SOC lookup', 'CV charging', 'Ready');
xlabel('Time (s)');
ylabel('current (mAh)');
title('CC/CV Charging Current');
xlim([0,1100]);

