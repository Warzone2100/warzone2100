SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Component.[Body Points],
       Component.[Graphics filename0],
       Sensor.[Mount Graphic],
       Sensor.[Sensor Range],
       Sensor.Position,
       Sensor.[Sensor Type],
       Sensor.[Sensor Time],
       Sensor.[Sensor Power],
       Sensor.design
FROM [Technology Type] INNER JOIN (Component INNER JOIN Sensor ON Component.[Component ID] = Sensor.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Two" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="Level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All"))
ORDER BY Component.[Component Name] DESC;
