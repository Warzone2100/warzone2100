-- vim:set et sts=2 sw=2:

dofile("patterns.lua")

-- A custom pattern
-- This pattern will be set when clicking once on 'myWindow'
pattern = betawidget.pattern("window/green", 0, 0, 0, 1)
pattern:addColourStop(0.00, 0.0000, 0.2353, 0.0353, 0.47)
pattern:addColourStop(1.00, 0.0000, 0.2353, 0.0353, 0.27)

wnd = betawidget.window("myWindow", 400, 400)
wnd:reposition(400, 50)
wnd:show()
wnd:addEventHandler(betawidget.EVT_MOUSE_CLICK,
-- Closure example: The variable `clicked' is bound to the closure of the
-- anonymous function which we return here.
  (function ()
    local clicked = 0
    return function (self, evt, handlerId)
      clicked = clicked + 1
      print(string.format("Clicked %d times", clicked))
      if (clicked % 2) == 0 then
        self:reposition(400, 50)
        self:setBackgroundPattern("window")
      else
        self:reposition(0, 0)
        self:setBackgroundPattern("window/green")
      end
    end
  end)()
)
wnd:addEventHandler(betawidget.EVT_KEY_DOWN,
  function (self, evt, handlerId)
    print("Key down")
    self:reposition(0, 0)
  end
)

wnd2 = betawidget.window("myOtherWindow", 200, 100)
wnd2:repositionFromAnchor(wnd, betawidget.CENTRE, 0, betawidget.MIDDLE, 0)
print(string.format("It is: %f %f", wnd2.offset.x, wnd2.offset.y))
wnd2:show()

btn = betawidget.button("myButton", 60, 40);
btn:show();
wnd:addChild(btn);
btn:reposition(40, 60)

imgButton = betawidget.imageButton("myImageButton", 60, 40)
imgButton:setImage(0, "tank.svg")
imgButton:show();
wnd2:addChild(imgButton);
imgButton:reposition(60, 20)

print(string.format("The clipboard's contents are \"%s\"", betawidget.getClipboardText() or ""))
betawidget.setClipboardText("Hello World")

-- A demonstration of destruction through the garbage collector
-- FIXME: The sequence of widget = nil; collectgarbage("collect"); should
--        really be replaced by something like widget:destroy(). This should
--        happen in such a way that it's still safe to access any still alive
--        references to this widget, e.g. it shouldn't result in
--        widgetDestroy(WIDGET(widget)) being called twice. We probably need
--        some way to turn these references into invalidated widget handles.
--[[
wnd:addTimerEventHandler(betawidget.EVT_TIMER_SINGLE_SHOT, 5000,
  function (self, evt, handlerId)
    print "collecting garbage: 1"
    collectgarbage("collect")
    self:addTimerEventHandler(betawidget.EVT_TIMER_SINGLE_SHOT, 5000,
      function (self, evt, handlerId)
        print "collecting garbage: 2"
        self = nil
        collectgarbage("collect")
      end
    )
  end
)
--]]

wnd3 = betawidget.window('test destroy window', 75, 37.5)
wnd3:reposition(50, 50)
wnd3:show()

wnd3:addTimerEventHandler(betawidget.EVT_TIMER_SINGLE_SHOT, 15000,
  function (self, evt, handlerId)
    print "destroying window 3"
    print "if you close this application now, you should experience a segfault"
    self:destroy()
  end
)

wnd3:addTimerEventHandler(betawidget.EVT_TIMER_PERSISTENT, 1000,
  (function ()
    local starttime = betawidget.getTime()
    return function (self, evt, handlerId)
      print(string.format("%f seconds passed", (betawidget.getTime() - starttime) / 1000))
    end
  end)()
)

wnd = nil
wnd2 = nil

scale = betawidget.animationScaleFrame
alpha = betawidget.animationAlphaFrame
translate = betawidget.animationTranslateFrame
rotate = betawidget.animationRotateFrame

wnd3:addAnimation({scale(0, 0, 0),
                   rotate(0, 90),
                   translate(0, 250, 0),
                   alpha(0, 0),
                   scale(1000, 1, 1),
                   rotate(1500, 0),
                   translate(2000, 50, 50),
                   alpha(3000, 1)})
