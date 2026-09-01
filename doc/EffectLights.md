# Effect lights

`stats/effectlights.json` sets the light that effects throw. (It is cosmetic - it changes only what is drawn.)

A missing file, a missing entry, or a missing field is normal.

Anything not set here uses the defaults.

## Projectiles

Example:
```json
{
	"projectiles": {
		"subclass": {
			"ROCKET": { "color": [255, 170, 90] }
		},
		"weapon": {
			"Rocket-LtA-T": { "rangeScale": 1.2 }
		}
	}
}
```

Each field is looked up on its own: the weapon's entry first, then its subclass, then the built-in default.
An entry that sets only a color still takes its reach from the model.

| Field | Type | Default |
|---|---|---|
| `enabled` | `true` / `false` | whether the in-flight graphic is drawn as a glow |
| `color` | `[r, g, b]`, each 0-255 | warm for kinetic weapons, cool for heat, orange for flame |
| `range` | 1 to 1024 | from the graphic's cross section |
| `intensity` | 0.01 to 4.0 | from the graphic's cross section |
| `rangeScale` | 0.05 to 4.0 | 1.0 |
| `intensityScale` | 0.05 to 4.0 | 1.0 |
| `fadeDuration` | 0 to 1000 | 200 |

`rangeScale` and `intensityScale` multiply whatever the reach and brightness would otherwise have been,
whether that came from `range` / `intensity` or from the graphic.

Subclass names are the ones `weapons.json` uses, such as `MACHINE GUN` and `SLOW ROCKET`.
Weapon names are the ids in `weapons.json`.
A key beginning with `_` is ignored, so `_comment` can be used for in-file notes.

## Picking values

A projectile light is on screen for a whole flight, often several at once.

Real emissive sources are near-white at their core, and most lights the game ships are warm or near-white.

A saturated hue often reads as a color filter over the ground rather than as light.

Brightness above about 2.0 does not look brighter. The accumulated light is scaled back by its strongest channel, so it flattens into a disk of flat color instead. Reach beyond about 512 is usually excessive, and may noticeably crowd muzzle flashes and explosions out of the point light budget.

`fadeDuration` is how long, in milliseconds, the light lingers and dims after the projectile stops (instead of vanishing in a single frame). Keep it short, no more than about 400 ms - a long fade leaves a phantom light where nothing is burning. Set it to 0 to turn the fade off for a weapon whose impact already throws its own light.

Run with `--debug=wz` to see the values every weapon ends up with and where each came from.

The script debugger's Graphics tab has a button to re-read the file.
