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
ORDER BY Component.[Component Name] DESC;
