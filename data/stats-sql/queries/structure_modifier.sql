SELECT bulletEffect.[bullet effect],
       [Structure Strength].[structure strength],
       [structure modifier].Modifier
FROM ([structure modifier] INNER JOIN bulletEffect ON [structure modifier].[weapon effect] = bulletEffect.id) INNER JOIN [Structure Strength] ON [structure modifier].[structure strength] = [Structure Strength].id;
