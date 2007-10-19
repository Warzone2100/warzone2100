SELECT Component.[Component Name],
       [Technology Type].[Tecnology Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Program.[Slots required],
       Program.OrderID,
       Program.[SpecialAbilityID Type]
FROM [Technology Type] INNER JOIN (Component INNER JOIN Program ON Component.[Component ID] = Program.[Component ID]) ON [Technology Type].[TechnologyType ID] = Component.[TechnologyType ID]
ORDER BY Component.[Component Name] DESC;
