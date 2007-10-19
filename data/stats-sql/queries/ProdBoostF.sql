SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Production Boost Function].Factory,
       [Production Boost Function].[Cyborg Factory],
       [Production Boost Function].[VTOL Factory],
       [Production Boost Function].[Production Output Modifier]
FROM [Function Types] INNER JOIN (Functions INNER JOIN [Production Boost Function] ON Functions.[Function ID] = [Production Boost Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type];
