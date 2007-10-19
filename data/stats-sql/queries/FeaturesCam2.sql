SELECT Features.[Feature Name],
       Features.[Base Width],
       Features.[Base Breadth],
       Features.damageable,
       Features.[Armour Value],
       Features.[Body Points],
       Features.[PSX Graphics filename],
       [Feature Types].FeatureType,
       Features.tiledraw,
       Features.allowLOS,
       Features.visibleAtStart
FROM [Technology Type] INNER JOIN ([Feature Types] INNER JOIN Features ON [Feature Types].ID = Features.FeatureType) ON [Technology Type].[TechnologyType ID] = Features.[TechnologyType ID]
WHERE ((([Technology Type].[Tecnology Name])="Level Two" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="Level Two-Three" Or ([Technology Type].[Tecnology Name])="Level All") AND ((Features.PSX)=Yes));
