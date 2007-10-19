SELECT DISTINCTROW Research.[Deliverance Name],
       Component.[Component Name],
       [Component Names].[Component Type],
       [Component Names].ID
FROM (([Component Names] INNER JOIN Component ON [Component Names].ID = Component.[Component Type]) INNER JOIN [Redundant CompList] ON Component.[Component ID] = [Redundant CompList].[Component ID]) INNER JOIN Research ON [Redundant CompList].[Research ID] = Research.ResearchID
WHERE (((DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=2 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=4 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=5 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=6))
ORDER BY Component.[Component Name];
