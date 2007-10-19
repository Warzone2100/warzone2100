SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       DLookUp("[Component Name]","Component","[Component ID] = " & [Weapon Stat]) AS Weapon,
       Brain.[Program Slots]
FROM [Technology Type] INNER JOIN (Component INNER JOIN Brain ON Component.[Component ID] = Brain.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
ORDER BY Component.[Component Name] DESC;
