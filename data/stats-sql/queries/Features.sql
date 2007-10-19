SELECT Features.[Feature Name],
       Features.[Base Width],
       Features.[Base Breadth],
       Features.damageable,
       Features.[Armour Value],
       Features.[Body Points],
       Features.[Graphics filename0],
       [Feature Types].FeatureType,
       Features.tiledraw,
       Features.allowLOS,
       Features.visibleAtStart
FROM [Feature Types] INNER JOIN Features ON [Feature Types].ID = Features.FeatureType;
