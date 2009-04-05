-- Default patterns for button widgets
-- Normal
pattern = betawidget.pattern("button/normal/fill", 0, 0, 1, 0)
pattern:addColourStop(0.00, 0.0471, 0.3843, 0.5647, 0.31)
pattern:addColourStop(0.00, 0.0235, 0.2667, 0.4000, 0.38)

pattern = betawidget.pattern("button/normal/contour", 0, 0, 1, 1)
pattern:addColourStop(0.00, 0.4000, 0.5373, 0.6275, 1.00)

-- Disabled
pattern = betawidget.pattern("button/disabled/fill", 0, 0, 1, 0)
pattern:addColourStop(0.00, 0.0863, 0.2039, 0.2667, 0.81)
pattern:addColourStop(1.00, 0.1137, 0.2392, 0.3098, 0.82)

pattern = betawidget.pattern("button/disabled/contour", 0, 0, 1, 1)
pattern:addColourStop(0.00, 0.5179, 0.6392, 0.7098, 0.35)

-- Mouse over
pattern = betawidget.pattern("button/mouseover/fill", 0, 0, 1, 0)
pattern:addColourStop(0.00, 0.5176, 0.7294, 0.8667, 0.25)
pattern:addColourStop(1.00, 0.3020, 0.7333, 1.0000, 0.20)

pattern = betawidget.pattern("button/mouseover/contour", 0, 0, 1, 1)
pattern:addColourStop(0.00, 0.4000, 0.5373, 0.6275, 1.00)

-- Mouse down
pattern = betawidget.pattern("button/mousedown/fill", 0, 0, 1, 0)
pattern:addColourStop(0.00, 0.0471, 0.3843, 0.5647, 0.80)
pattern:addColourStop(1.00, 0.0235, 0.2666, 0.4000, 0.88)

pattern = betawidget.pattern("button/mousedown/contour", 0, 0, 1, 1)
pattern:addColourStop(0.00, 0.4000, 0.5373, 0.6275, 0.50)

-- Default window pattern
-- The pattern "window" is used in betawidget.window by default
pattern = betawidget.pattern("window", 0, 0, 0, 1)
pattern:addColourStop(0.00, 0.0039, 0.2667, 0.4039, 0.49)
pattern:addColourStop(1.00, 0.0000, 0.1529, 0.2353, 0.47)
