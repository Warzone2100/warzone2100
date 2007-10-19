SELECT DISTINCTROW Propulsion.[Component ID],
       Component.[Component Name],
       Propulsion.[Propulsion Type ID]
FROM Component INNER JOIN Propulsion ON Component.[Component ID] = Propulsion.[Component ID]
ORDER BY Component.[Component Name];
