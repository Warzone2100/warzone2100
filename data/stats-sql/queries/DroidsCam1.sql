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
FROM [Technology Type] INNER JOIN (DroidType INNER JOIN Droids ON DroidType.id = Droids.[Droid Type]) ON [Technology Type].[TechnologyType ID] = Droids.[TechnologyType ID]
WHERE (((Droids.[Droid Name])<>"None") AND ((Droids.[Player ID])=0 Or (Droids.[Player ID])=1 Or (Droids.[Player ID])=2 Or (Droids.[Player ID])=3 Or (Droids.[Player ID])=7) AND (([Technology Type].[Tecnology Name])="Level One" Or ([Technology Type].[Tecnology Name])="Level One-Two" Or ([Technology Type].[Tecnology Name])="Level All"));
