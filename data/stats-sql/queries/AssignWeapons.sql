SELECT Droids.[Droid Name],
       Component.[Component Name],
       Droids.[Player ID]
FROM Component INNER JOIN (Weapons INNER JOIN (Droids INNER JOIN AssignWeapons ON Droids.DroidID = AssignWeapons.[Droid ID]) ON Weapons.[Component ID] = AssignWeapons.[Weapon ID]) ON Component.[Component ID] = Weapons.[Component ID]
ORDER BY Droids.[Droid Name];
