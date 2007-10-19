SELECT DISTINCTROW Research.[Deliverance Name],
       Component.[Component Name],
       [Component Names].[Component Type],
       [Component Names].ID
FROM (([Component Names] INNER JOIN Component ON [Component Names].ID = Component.[Component Type]) INNER JOIN [Redundant CompList] ON Component.[Component ID] = [Redundant CompList].[Component ID]) INNER JOIN Research ON [Redundant CompList].[Research ID] = Research.ResearchID
ORDER BY Component.[Component Name];
