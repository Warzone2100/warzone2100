SELECT [Function Types].[Function Type],
       Functions.[Function Name],
       WeaponSubClass.[Weapon SubClass Name],
       [Weapon Upgrade Function].[Rate Of Fire],
       [Weapon Upgrade Function].[short range accuracy],
       [Weapon Upgrade Function].[long range accuracy],
       [Weapon Upgrade Function].Damage,
       [Weapon Upgrade Function].[Explosive Damage],
       [Weapon Upgrade Function].[Incendiary Damage],
       [Weapon Upgrade Function].[Explosive Hit Chance]
FROM (WeaponSubClass INNER JOIN ([Weapon Upgrade Function] INNER JOIN Functions ON [Weapon Upgrade Function].[Function ID] = Functions.[Function ID]) ON WeaponSubClass.id = [Weapon Upgrade Function].[Weapon SubClass]) INNER JOIN [Function Types] ON Functions.[Function Type] = [Function Types].ID;
