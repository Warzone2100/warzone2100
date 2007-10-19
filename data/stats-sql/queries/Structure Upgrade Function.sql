SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       [Structure Upgrade Function].[Armour Points],
       [Structure Upgrade Function].[Body Points],
       [Structure Upgrade Function].[Resistance Points]
FROM (Functions INNER JOIN [Function Types] ON Functions.[Function Type] = [Function Types].ID) INNER JOIN [Structure Upgrade Function] ON Functions.[Function ID] = [Structure Upgrade Function].[Function ID];
