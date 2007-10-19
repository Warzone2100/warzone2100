SELECT DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]) AS Owner,
       DLookUp("[Structures]![Structure Name]","[Structures]","[Structures]![StructureID] =" & [StructureID]) AS Structure,
       ResStr([replaced StructID]) AS ReplacedStructure,
       [Result Structure List].StructureID
FROM [Result Structure List] INNER JOIN Research ON [Result Structure List].[Research ID] = Research.ResearchID
ORDER BY DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]);
