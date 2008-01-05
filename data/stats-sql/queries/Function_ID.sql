SELECT DISTINCTROW Functions.[Function ID],
       Functions.[Function Name],
       [Function Types].[Function Type],
       Functions.Usage
FROM [Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]
WHERE ((([Function Types].[Function Type])<>"Defensive Structure") AND ((Functions.Usage)="STRUCTURE"));
