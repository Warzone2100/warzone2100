SELECT Droids.[Droid Name],
       Component.[Component Name],
       Droids.[Player ID]
FROM (Component INNER JOIN Program ON Component.[Component ID] = Program.[Component ID]) INNER JOIN (Droids INNER JOIN AssignPrograms ON Droids.DroidID = AssignPrograms.[Droid ID]) ON Program.[Component ID] = AssignPrograms.[Program ID]
ORDER BY Droids.[Droid Name];
