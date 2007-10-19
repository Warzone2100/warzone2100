SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Research Upgrade Function].[Research Points]
FROM [Function Types] INNER JOIN (Functions INNER JOIN [Research Upgrade Function] ON Functions.[Function ID] = [Research Upgrade Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type];
