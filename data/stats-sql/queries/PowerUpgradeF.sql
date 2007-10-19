SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Power Upgrade Function].[Power Modifier]
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [Power Upgrade Function] ON Functions.[Function ID] = [Power Upgrade Function].[Function ID];
