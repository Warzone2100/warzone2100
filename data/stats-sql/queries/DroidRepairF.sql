SELECT DISTINCTROW
       [Function Types].[Function Type],
       Functions.[Function Name],
       [Droid Repair Function].[Repair Facility Points]
FROM [Function Types] INNER JOIN (Functions INNER JOIN [Droid Repair Function] ON Functions.[Function ID] = [Droid Repair Function].[Function ID]) ON [Function Types].ID = Functions.[Function Type];
