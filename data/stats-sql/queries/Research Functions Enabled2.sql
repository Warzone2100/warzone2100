SELECT DISTINCTROW Research.[Deliverance Name],
       Functions.[Function Name],
       [Functions Enabled].[Research ID]
FROM Research INNER JOIN (([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [Functions Enabled] ON Functions.[Function ID] = [Functions Enabled].[Function ID]) ON Research.ResearchID = [Functions Enabled].[Research ID]
WHERE (((DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=2 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=4 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=5 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=6));
