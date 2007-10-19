SELECT DISTINCTROW Sensor.[Component ID],
       Component.[Component Name],
       Sensor.Position
FROM Component INNER JOIN Sensor ON Component.[Component ID] = Sensor.[Component ID];
