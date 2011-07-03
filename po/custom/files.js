[
    {"push": true,
     "type": "ini",
     "file": "../icons/warzone2100.desktop",
     "output": "warzone2100.desktop.txt",
     "strings": [
        ["Desktop Entry", "Name"],
        ["Desktop Entry", "GenericName"],
        ["Desktop Entry", "Comment"]]
    },
    {"push": false,
     "type": "mac",
     "file": "../macosx/Resources/wzlocal/English.lproj/InfoPlist.strings",
     "output": "mac-infoplist.txt",
     "pushpath": "../macosx/Resources/wzlocal/%s.lproj/InfoPlist.strings",
     "strings": [
    	["CFBundleName", false],
    	["CFBundleDisplayName", false],
    	["NSHumanReadableCopyright", false],
    	["Warzone 2100 Map / Mod File", true],
    	["Warzone 2100 Multiplayer Mod File", true],
    	["Warzone 2100 Campaign Mod File", true],
    	["Warzone 2100 Global Mod File", true],
    	["Warzone 2100 Music Mod File", true]]
    }
]
