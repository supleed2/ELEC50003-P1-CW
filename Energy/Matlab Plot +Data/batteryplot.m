data = csvread ( "charge.csv");
soc1 = data(:,6);
voltage1 = data(:,2);

data = csvread ( "discharge.csv");
soc2 = data(:,6);
voltage2 = data(:,2);


figure;
hold on;
plot(soc1, voltage1, 'x');

plot(soc2, voltage2, 'x');

xlabel('SOC (%)');
ylabel('Voltage (mV)');
title('SOC of a battery when charging and discharging ');
set(gca,'FontSize',15);
legend( 'charge', 'discharge');
hold off;