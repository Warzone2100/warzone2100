SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Component.[Body Points],
       Component.[Graphics filename0],
       ECM.[Mount Graphic],
       ECM.Position,
       ECM.[ECM Modifier],
       ECM.design
FROM [Technology Type] INNER JOIN (Component INNER JOIN ECM ON Component.[Component ID] = ECM.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level One" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="Level All"))
ORDER BY Component.[Component Name] DESC;
