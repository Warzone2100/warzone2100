SELECT bulletEffect.[bullet effect],
       [Propulsion Type].[Propulsion Name],
       [Weapon Modifier].Modifier
FROM [Propulsion Type] INNER JOIN ([Weapon Modifier] INNER JOIN bulletEffect ON [Weapon Modifier].[weapon effect] = bulletEffect.id) ON [Propulsion Type].[Propulsion Type ID] = [Weapon Modifier].[Propulsion Type ID];
