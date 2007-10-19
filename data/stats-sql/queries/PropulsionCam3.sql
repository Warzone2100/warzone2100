SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Component.[Body Points],
       Component.[Graphics filename0],
       [Propulsion Type].[Propulsion Name],
       Propulsion.[Maximum Speed],
       Propulsion.design
FROM ([Technology Type] INNER JOIN (Component INNER JOIN Propulsion ON Component.[Component ID] = Propulsion.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]) INNER JOIN [Propulsion Type] ON Propulsion.[Propulsion Type ID] = [Propulsion Type].[Propulsion Type ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Three" Or ([Technology Type].[Tecnology Name])="Level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All"))
ORDER BY Component.[Component Name] DESC;
