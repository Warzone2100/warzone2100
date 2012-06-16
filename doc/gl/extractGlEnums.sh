grep -o -I -r -P "[(,]\s*GL_[A-Z_0-9]+" src lib | sed "/~/d" | sed "/betawidget/d"
