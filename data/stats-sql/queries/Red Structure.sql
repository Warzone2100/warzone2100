SELECT DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]) AS Owner,
       DLookUp("[Structures]![Structure Name]","[Structures]","[Structures]![StructureID] =" & [StructureID]) AS Structure,
       [Redundant  StrList].StructureID
FROM Research INNER JOIN [Redundant  StrList] ON Research.ResearchID = [Redundant  StrList].[Research ID]
ORDER BY DLookUp("[Research]![Deliverance Name]","[Research]","[Research]![ResearchID] = " & [Research ID]);
