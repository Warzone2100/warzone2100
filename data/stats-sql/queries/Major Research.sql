SELECT Research.ResearchID,
       Research.[Technology Code],
       Research.[WIP Name],
       Research.[Deliverance Name]
FROM Research
WHERE (((Research.[Technology Code])=0));
