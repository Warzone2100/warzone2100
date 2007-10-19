SELECT DISTINCTROW Component.[Component ID],
       Component.[Component Name],
       ECM.[ECM Modifier]
FROM Component INNER JOIN ECM ON Component.[Component ID] = ECM.[Component ID];
