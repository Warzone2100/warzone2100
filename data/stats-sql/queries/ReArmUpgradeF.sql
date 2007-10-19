SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [ReArm Upgrade Function].[ReArm Points]
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [ReArm Upgrade Function] ON Functions.[Function ID] = [ReArm Upgrade Function].[Function ID];
