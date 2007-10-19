SELECT DISTINCTROW
       Component.[Component ID],
       Component.[Component Name],
       Component.[Component Type],
       [Technology Type].[Tecnology Name]
FROM ([Component Names] INNER JOIN Component ON [Component Names].ID = Component.[Component Type]) INNER JOIN [Technology Type] ON Component.[TechnologyType ID] = [Technology Type].[TechnologyType ID]
WHERE (((Component.[Component Type])=2))
ORDER BY Component.[Component Name];
