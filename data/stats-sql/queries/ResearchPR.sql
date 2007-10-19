SELECT DISTINCTROW DLookUp("Research![Deliverance Name]","Research","Research![ResearchID] = " & [Research ID]) AS Owner,
       DLookUp("Research![Deliverance Name]","Research","Research![ResearchID] = " & [PRResearchID]) AS [pre-req],
       ResearchPR.[Research ID]
FROM ResearchPR
ORDER BY DLookUp("Research![Deliverance Name]","Research","Research![ResearchID] = " & [Research ID]);
