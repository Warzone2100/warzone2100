SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [VehicleUpgrade Function].[Repair Points]
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [VehicleUpgrade Function] ON Functions.[Function ID] = [VehicleUpgrade Function].[Function ID]
WHERE ((([Function Types].[Function Type])="VehicleECM Upgrade"));
