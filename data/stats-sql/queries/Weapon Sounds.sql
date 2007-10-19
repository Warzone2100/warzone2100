SELECT Component.[Component Name],
       [Weapon Sounds].[Firing Sound],
       [Weapon Sounds].[Explosion Sound],
       [Weapon Sounds].id
FROM [Weapon Sounds] INNER JOIN Component ON [Weapon Sounds].[Component ID] = Component.[Component ID];
