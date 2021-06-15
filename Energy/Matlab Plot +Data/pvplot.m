data = csvread ( "mpptlast.csv");
voltage = data(:,2);
current = data(:,3);

x = 0:459

figure;
plot(x, voltage, 'x');
xlabel(' (%)');
ylabel('Time (S)');
ylim[5500
title('MPPT');
set(gca,'FontSize',25);

