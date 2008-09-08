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
      else
        self:reposition(0, 0)
      end
    end
  end)()
)
wnd:addTimerEventHandler(betawidget.EVT_TIMER_PERSISTENT, 1000,
  (function ()
    local time = 0
    return function (self, evt, handlerId)
      time = time + 1
      print(string.format("%d seconds passed", time))
    end
  end)()
)
wnd:addEventHandler(betawidget.EVT_KEY_DOWN,
  function (self, evt, handlerId)
    print("Key down")
    self:reposition(0, 0)
  end
)

wnd2 = betawidget.window("myOtherWindow", 100, 100)
wnd2:repositionFromAnchor(wnd, betawidget.CENTRE, 0, betawidget.MIDDLE, 0)
print(string.format("It is: %f %f", wnd2.offset.x, wnd2.offset.y))
wnd2:show()

print(string.format("The clipboard's contents are \"%s\"", betawidget.getClipboardText() or ""))
betawidget.setClipboardText("Hello World")
