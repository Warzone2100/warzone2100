SELECT DISTINCTROW Structures.[Structure Name],
       Component.[Component Name],
       [Structure Weapons].ID
FROM (Component INNER JOIN (Weapons INNER JOIN (Structures INNER JOIN [Structure Weapons] ON Structures.StructureID = [Structure Weapons].StructureID) ON Weapons.[Component ID] = [Structure Weapons].[Weapon ID]) ON Component.[Component ID] = Weapons.[Component ID]) INNER JOIN [Technology Type] ON Structures.[TechnologyType ID] = [Technology Type].[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Three" Or ([Technology Type].[Tecnology Name])="Level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All") AND ((Structures.psx)=Yes));
