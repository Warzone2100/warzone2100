SELECT SpecialAbility.SpecialAbility,
       SpecialAbility.SpecialAbilityID
FROM SpecialAbility
WHERE (((SpecialAbility.SpecialAbility)<>"None"))
ORDER BY SpecialAbility.SpecialAbilityID;
