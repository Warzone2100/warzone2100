SELECT [Propulsion Type].[Propulsion Name],
       [Propulsion Sounds].[Start Sound],
       [Propulsion Sounds].[Idle Sound],
       [Propulsion Sounds].[Move Off Sound],
       [Propulsion Sounds].[Move Sound],
       [Propulsion Sounds].[Hiss Sound],
       [Propulsion Sounds].[Shut Down Sound],
       [Propulsion Sounds].id
FROM [Propulsion Sounds] INNER JOIN [Propulsion Type] ON [Propulsion Sounds].[Component ID] = [Propulsion Type].[Propulsion Type ID];
