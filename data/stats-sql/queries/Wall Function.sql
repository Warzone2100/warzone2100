SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       Structures.[Structure Name],
       [Wall Function].[Function ID]
FROM Structures INNER JOIN ([Function Types] INNER JOIN ([Wall Function] INNER JOIN Functions ON [Wall Function].[Function ID] = Functions.[Function ID]) ON [Function Types].ID = Functions.[Function Type]) ON Structures.StructureID = [Wall Function].[Corner Stat];
