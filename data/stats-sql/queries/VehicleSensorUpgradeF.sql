SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [VehicleSensor Upgrade Function].Power,
       [VehicleSensor Upgrade Function].Range
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [VehicleSensor Upgrade Function] ON Functions.[Function ID] = [VehicleSensor Upgrade Function].[Function ID];
