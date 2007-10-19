SELECT DISTINCTROW Research.[Deliverance Name],
       Component.[Component Name],
       [Component Names].[Component Type],
       [Component Names].ID
FROM [Component Names] INNER JOIN (Research INNER JOIN (Component INNER JOIN [PR Component List] ON Component.[Component ID] = [PR Component List].[Component ID]) ON Research.ResearchID = [PR Component List].[Research ID]) ON [Component Names].ID = Component.[Component Type]
ORDER BY Component.[Component Name];
