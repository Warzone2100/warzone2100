SELECT DLookUp("[bodynameQuery]![Component Name]","[bodynameQuery]","[bodynameQuery]![Component ID] = " & [Body ID]) AS BodyName,
       DLookUp(" [propnameQuery]![Component Name]","[propnameQuery]","[propnameQuery]![Component ID] = " & [Propulsion ID]) AS PropName,
       testTable.[left graphics file],
       testTable.[right graphics file],
       testTable.id
FROM testTable;
