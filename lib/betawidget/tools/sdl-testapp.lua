wnd = betawidget.window("myWindow", 400, 400)
wnd:reposition(400, 50)
wnd:show()
wnd:addEventHandler(betawidget.EVT_MOUSE_CLICK,
  function (self, evt, handlerId)
      print("Clicked")
      self:reposition(0, 0)
  end
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
