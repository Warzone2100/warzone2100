SELECT DISTINCTROW Research.[Deliverance Name],
       Component.[Component Name],
       [Component Names].[Component Type],
       componentName([Replaced CompID]) AS replacedComp,
       componentType([Replaced CompID]) AS replacedType,
       [Component Names].ID
FROM (([Component Names] INNER JOIN Component ON [Component Names].ID = Component.[Component Type]) INNER JOIN [Result Component List] ON Component.[Component ID] = [Result Component List].[Component ID]) INNER JOIN Research ON [Result Component List].[Research ID] = Research.ResearchID
WHERE (((Research.multiPlayer)=Yes))
ORDER BY Component.[Component Name];
