SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       BodySIze.[Body Size],
       [Production Function].[Production Output]
FROM BodySIze INNER JOIN ([Propulsion Type] INNER JOIN ([Function Types] INNER JOIN (Functions INNER JOIN [Production Function] ON Functions.[Function ID] = [Production Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type]) ON [Propulsion Type].[Propulsion Type ID] = [Production Function].[Propulsion Type ID]) ON ([Function Types].ID = BodySIze.id) AND (BodySIze.id = [Production Function].[Production capacity]);
