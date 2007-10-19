SELECT DISTINCTROW
       Construction.[Component ID],
       Component.[Component Name]
FROM Component INNER JOIN Construction ON Component.[Component ID] = Construction.[Component ID];
