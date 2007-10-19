SELECT DISTINCTROW Weapons.[Component ID],
       Component.[Component Name],
       [Technology Type].[Tecnology Name]
FROM (Component INNER JOIN Weapons ON Component.[Component ID] = Weapons.[Component ID]) INNER JOIN [Technology Type] ON Component.[TechnologyType ID] = [Technology Type].[TechnologyType ID]
ORDER BY Component.[Component Name];
