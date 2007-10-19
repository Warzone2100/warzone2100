SELECT [Propulsion Type].[Propulsion Name],
       [Propulsion Type].[Ground Flag],
       [Propulsion Type].[Power Ratio Multiplier]
FROM [Propulsion Type]
WHERE ((([Propulsion Type].[Propulsion Name])<>"None"));
