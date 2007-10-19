SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Repair Upgrade Function].[Repair Points]
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [Repair Upgrade Function] ON Functions.[Function ID] = [Repair Upgrade Function].[Function ID];
