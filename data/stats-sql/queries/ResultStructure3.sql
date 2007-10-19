SELECT DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]) AS Owner,
       DLookUp("[Structures]![Structure Name]","[Structures]","[Structures]![StructureID] =" & [StructureID]) AS Structure,
       ResStr([replaced StructID]) AS ReplacedStructure,
       [Result Structure List].StructureID
FROM [Result Structure List] INNER JOIN Research ON [Result Structure List].[Research ID] = Research.ResearchID
WHERE (((DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=3 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=5 Or (DLookUp(" [Research]![TechnologyType ID]","Research","Research![ResearchID] = " & [Research ID]))=6))
ORDER BY DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]);
