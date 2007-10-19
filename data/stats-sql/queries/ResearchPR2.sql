SELECT DLookUp("Research![Deliverance Name]","Research","Research![ResearchID] = " & [Research ID]) AS Owner,
       DLookUp("Research![Deliverance Name]","Research","Research![ResearchID] = " & [PRResearchID]) AS [pre-req],
       ResearchPR.PRResearchID
FROM ResearchPR
WHERE (((DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=2 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=5));
