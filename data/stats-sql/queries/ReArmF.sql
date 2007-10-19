SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       [ReArm Function].[ReArm Points]
FROM (Functions INNER JOIN [Function Types] ON Functions.[Function Type] = [Function Types].ID) INNER JOIN [ReArm Function] ON Functions.[Function ID] = [ReArm Function].[Function ID];
