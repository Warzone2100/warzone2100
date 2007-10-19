SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Repair.[Armour Repair],
       Repair.Position,
       Component.[Graphics filename0],
       Repair.[Mount Graphic],
       Repair.[Repair System],
       Repair.time,
       Repair.design
FROM [Technology Type] INNER JOIN (Component INNER JOIN Repair ON Component.[Component ID] = Repair.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
ORDER BY Component.[Component Name] DESC;
