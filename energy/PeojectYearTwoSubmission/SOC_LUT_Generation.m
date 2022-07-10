
close all;
voltage = readmatrix('BATCYCLECharacteristic.xls', 'Range','I5:I8627');
charge = readmatrix('BATCYCLECharacteristic.xls', 'Range','J5:J8627');

newVoltage = [];
newCharge = [];
consecutiveCount = 1;
i = 2;


%%this if statement removes oscillation around a voltage
for m = 2:length(voltage)
   if (voltage(m) <= voltage(m-1))     
        voltage(m) = voltage(m-1);
   end
end

while (i <= length(voltage))
       
    
       if (voltage(i-1) == voltage(i))
           consecutiveCount = consecutiveCount + 1;
       else
           newVoltage = [newVoltage, voltage(i-round(consecutiveCount/2))];
           newCharge = [newCharge, charge(i-round(consecutiveCount/2))];
           disp(consecutiveCount);
           consecutiveCount = 1;
       end
       
    
       
    i = i+1;
    
end
    
%plot(newVoltage, newCharge, '*')
hold on;

newInterpolatedPointsVoltage = 2990:2:3600;
newChargeInterpolated = interp1(newVoltage, newCharge, newInterpolatedPointsVoltage);
plot(newInterpolatedPointsVoltage, newChargeInterpolated, '.');
xlabel("Voltage (mV)");
ylabel("SOC (%)");
title("cell SOC VS Charging Voltage (charging at 250mA)")


fprintf('{')
for j = 1:length(newInterpolatedPointsVoltage)
   fprintf(string(newChargeInterpolated(j)));
   fprintf(', ')
end
fprintf('}')