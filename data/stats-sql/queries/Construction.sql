SELECT DISTINCTROW
       Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Component.[Body Points],
       Component.[Graphics filename0],
       Construction.[Mount Graphic],
       Construction.[Construction Points],
       Construction.design
FROM [Technology Type] INNER JOIN (Component INNER JOIN Construction ON Component.[Component ID] = Construction.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID];
