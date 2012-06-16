grep -P '[^a-zA-Z0-9]gl[A-Z][a-zA-Z0-9]+\(' -o -r src lib | sed "/~/d" | sed "s/:\//:\ /"| sed "s/:(/: /" | sed "s/:)/: /" | sed "s/:\!/: /" | sed "s/:_/: /" | sed "/betawidget/d"
