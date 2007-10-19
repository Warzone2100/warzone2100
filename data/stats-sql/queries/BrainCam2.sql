SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       DLookUp("[Component Name]","Component","[Component ID] = " & [Weapon Stat]) AS Weapon, Brain.[Program Slots]
FROM [Technology Type] INNER JOIN (Component INNER JOIN Brain ON Component.[Component ID] = Brain.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Two" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All"))
ORDER BY Component.[Component Name] DESC;
