SELECT DLookUp("[bodynameQuery]![Component Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]) AS BodyName,
       DLookUp(" [propnameQuery]![Component Name]","[propnameQuery]","[propnameQuery]![Component ID] = " & [Propulsion ID]) AS PropName,
       testTable.[left graphics file],
       testTable.[right graphics file],
       testTable.id
FROM testTable
WHERE (((DLookUp("[bodynameQuery]![Tecnology Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]))="Level Two" Or (DLookUp("[bodynameQuery]![Tecnology Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]))="Level One-Two" Or (DLookUp("[bodynameQuery]![Tecnology Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]))="Level Two-Three" Or (DLookUp("[bodynameQuery]![Tecnology Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]))="Level All"));
