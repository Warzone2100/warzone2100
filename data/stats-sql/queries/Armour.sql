SELECT Component.[Component Name],
       Component.[Power Required],
       Component.[Build Points Required],
       Component.Weight,
       Component.[Hit Points],
       Component.[System Points],
       Armour.[Armour Value]
FROM Component INNER JOIN Armour ON Component.[Component ID] = Armour.[Component ID]
ORDER BY Component.[Component Name] DESC;
