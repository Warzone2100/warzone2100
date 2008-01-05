SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       [WallDefence Upgrade Function].[Armour Points],
       [WallDefence Upgrade Function].[Body Points]
FROM (Functions INNER JOIN [Function Types] ON Functions.[Function Type] = [Function Types].ID) INNER JOIN [WallDefence Upgrade Function] ON Functions.[Function ID] = [WallDefence Upgrade Function].[Function ID];
