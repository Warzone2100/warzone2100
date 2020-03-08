The `win_installer*.nsh` files in this folder are used to define localized additional
strings for the NSIS installers.

The default / base language file is English, at:
[**`win_installer_base.nsh`**](win_installer_base.nsh).
It **must** contain _all_ required strings, and is used as a fallback if other
languages are missing translations / strings.

See: **[/doc/Translations.md](/doc/Translations.md)** for details on how to provide / edit translations.

---

`*.nsh` Language Files Header:

```
;  This file is part of Warzone 2100.
;  Copyright (C) 2006-2020  Warzone 2100 Project
;  Copyright (C) 2006       Dennis Schridde
;
;  Warzone 2100 is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 2 of the License, or
;  (at your option) any later version.
;
;  Warzone 2100 is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with Warzone 2100; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
;
;  NSIS Modern User Interface
;  Warzone 2100 Project Installer script
;
```
