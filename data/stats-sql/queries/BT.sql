SELECT DISTINCTROW
       Component.[Component ID],
       Component.[Component Name],
       Body.[Power Output]
FROM Component INNER JOIN Body ON Component.[Component ID] = Body.[Component ID];
