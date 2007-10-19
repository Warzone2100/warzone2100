SELECT Droids.[Droid Name],
       Droids.DroidID,
       DLookUp("[Component Name]","Component","[Component ID] = " & [BodyType]) AS Body,
       DLookUp("[Component Name]","Component","[Component ID] = " & [BrainType]) AS Brain,
       DLookUp("[Component Name]","Component","[Component ID] = " & [ConstructionType]) AS Construction,
       DLookUp("[Component Name]","Component","[Component ID] = " & [ECMType]) AS ECM,
       Droids.[Player ID],
       DLookUp("[Component Name]","Component","[Component ID] = " & [PropulsionType]) AS Prop,
       DLookUp("[Component Name]","Component","[Component ID] = " & [RepairType]) AS Repair,
       DroidType.[Droid Type],
       DLookUp("[Component Name]","Component","[Component ID] = " & [SensorType]) AS Sensor,
       DCount("[Droid ID]","AssignWeapons","[Droid ID] = " & [DroidID]) AS TotalWeapons
FROM DroidType INNER JOIN Droids ON DroidType.id = Droids.[Droid Type]
WHERE (((Droids.[Droid Name])<>"None"));
