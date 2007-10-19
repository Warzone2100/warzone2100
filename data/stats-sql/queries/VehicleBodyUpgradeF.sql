SELECT DISTINCTROW [Function Types].[Function Type],
       Functions.[Function Name],
       [VehicleBodyUpgrade Function].Power,
       [VehicleBodyUpgrade Function].bodyPoints,
       [VehicleBodyUpgrade Function].[Kinetic Armour Value],
       [VehicleBodyUpgrade Function].[HeatArmour Value],
       [VehicleBodyUpgrade Function].vehicle,
       [VehicleBodyUpgrade Function].cyborg
FROM ([Function Types] INNER JOIN Functions ON [Function Types].ID = Functions.[Function Type]) INNER JOIN [VehicleBodyUpgrade Function] ON Functions.[Function ID] = [VehicleBodyUpgrade Function].[Function ID];
