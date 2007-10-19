SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [HQ Function].Power
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [HQ Function] ON Functions.[Function ID] = [HQ Function].[Function ID];
