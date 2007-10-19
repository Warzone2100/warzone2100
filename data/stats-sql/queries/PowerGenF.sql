SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [Power Generator Function].[Power Output],
       [Power Generator Function].[Power Range],
       [Power Generator Function].[Critical Mass Chance],
       [Power Generator Function].[Critical Mass Radius],
       [Power Generator Function].[Critical Mass Damage],
       [Power Generator Function].[Radiation Decay]
FROM [Function Types] INNER JOIN (Functions INNER JOIN [Power Generator Function] ON Functions.[Function ID] = [Power Generator Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type];
