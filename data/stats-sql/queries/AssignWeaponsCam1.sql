SELECT Droids.[Droid Name],
       Component.[Component Name],
       Droids.[Player ID]
FROM (Component INNER JOIN (Weapons INNER JOIN (Droids INNER JOIN AssignWeapons ON Droids.DroidID = AssignWeapons.[Droid ID]) ON Weapons.[Component ID] = AssignWeapons.[Weapon ID]) ON Component.[Component ID] = Weapons.[Component ID]) INNER JOIN [Technology Type] ON Droids.[TechnologyType ID] = [Technology Type].[TechnologyType ID]
WHERE (((Droids.[Player ID])=0 Or (Droids.[Player ID])=1 Or (Droids.[Player ID])=2 Or (Droids.[Player ID])=3 Or (Droids.[Player ID])=7) AND (([Technology Type].[Tecnology Name])="Level One" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="Level All"))
ORDER BY Droids.[Droid Name];
