SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       [Research Function].[Research Points]
FROM (Functions INNER JOIN [Research Function] ON Functions.[Function ID] = [Research Function].[Function ID]) INNER JOIN [Function Types] ON Functions.[Function Type] = [Function Types].ID;
