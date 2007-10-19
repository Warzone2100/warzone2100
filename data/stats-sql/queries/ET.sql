SELECT DISTINCTROW ECM.[Component ID],
       Component.[Component Name],
       ECM.Position
FROM Component INNER JOIN ECM ON Component.[Component ID] = ECM.[Component ID];
