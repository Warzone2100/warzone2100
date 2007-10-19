SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Power Regulator Function].[Max Power]
FROM [Function Types] INNER JOIN (Functions INNER JOIN [Power Regulator Function] ON Functions.[Function ID] = [Power Regulator Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type];
