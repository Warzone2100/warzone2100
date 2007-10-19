SELECT DISTINCTROW Structures.[Structure Name],
       Functions.[Function Name],
       [Structure Functions].ID
FROM Functions INNER JOIN (Structures INNER JOIN [Structure Functions] ON Structures.StructureID = [Structure Functions].StructureID) ON Functions.[Function ID] = [Structure Functions].[Function ID];
