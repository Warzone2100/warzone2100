SELECT DISTINCTROW Component.[Component ID],
       Component.[Component Name],
       Sensor.[Sensor Range],
       Sensor.[Sensor Power]
FROM Component INNER JOIN Sensor ON Component.[Component ID] = Sensor.[Component ID];
