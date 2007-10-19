SELECT DISTINCTROW Component.[Component ID],
       Component.[Component Name],
       Component.[Component Type]
FROM Component
WHERE (((Component.[Component Type])=5))
ORDER BY Component.[Component Name];
