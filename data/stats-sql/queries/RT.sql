SELECT DISTINCTROW Repair.[Component ID],
       Component.[Component Name]
FROM Component INNER JOIN Repair ON Component.[Component ID] = Repair.[Component ID];
