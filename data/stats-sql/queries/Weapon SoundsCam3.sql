SELECT Component.[Component Name],
       [Weapon Sounds].[Firing Sound],
       [Weapon Sounds].[Explosion Sound],
       [Weapon Sounds].id
FROM ([Weapon Sounds] INNER JOIN Component ON [Weapon Sounds].[Component ID] = Component.[Component ID]) INNER JOIN [Technology Type] ON Component.[TechnologyType ID] = [Technology Type].[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Three" Or ([Technology Type].[Tecnology Name])="Level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All"));
