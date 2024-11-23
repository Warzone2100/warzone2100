// NOTE: This config should be run on the *installed* files
module.exports = {
	// -----------------------------
	// Pre-cache configuration
	globDirectory: './',
	globPatterns: [
		'index.html',
		'manifest.json',
		// warzone2100.js, warzone2100.worker.js
		'*.js',
		// Core WASM file
		'warzone2100.wasm',
		// Specific favicon assets
		'assets/favicon-16x16.png',
		'assets/favicon-32x32.png',
		'assets/android-chrome-192x192.png',
		// Any remaining css, js, or json files from the assets directory
		'assets/*.{css,js,json}'
	],
	globIgnores: [
		'**\/node_modules\/**\/*',
		'**/service-worker.js', // do not precache the service-worker file (which shouldn't exist when this is run, but just in case)
		'**/extern-postjs.js', // do not precache generated extern-postjs.js file (ends up included in build)
		'**/*.data', // do not precache .data files, which are often huge
		'**/*.debug.wasm', // do not precache wasm debug symbols
		'**\/music\/**\/*', // do not precache music (which is optional)
		'**\/terrain_overrides\/**\/*' // do not precache terrain_overrides (which are optional)
	],
	// NOTE: These should match the versions used in shell.html!
	additionalManifestEntries: [
		{ url: 'https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/css/bootstrap.min.css', revision: null, integrity: 'sha512-b2QcS5SsA8tZodcDtGRELiGv5SaKSk1vDHDaQRda0htPYWZ6046lr3kJ5bAAQdpV2mmA/4v0wQF9MyU6/pDIAg==' },
		{ url: 'https://cdnjs.cloudflare.com/ajax/libs/bootstrap/5.3.2/js/bootstrap.bundle.min.js', revision: null, integrity: 'sha512-X/YkDZyjTf4wyc2Vy16YGCPHwAY8rZJY+POgokZjQB2mhIRFJCckEGc6YyX9eNsPfn0PzThEuNs+uaomE5CO6A==' }
	],
	maximumFileSizeToCacheInBytes: 104857600, // Must be greater than the file size of any precached files
	// -----------------------------
	// Runtime caching configuration
	runtimeCaching: [
		// NOTE: In regards to warzone2100.data:
		//   - We do not want to precache it, because it's huge (and precaching doesn't let us show a nice progress bar currently)
		//   - It is version/build-specific, and should always be in sync with the other data / files
		//   - It is already cached by the Emscripten preload cache, which handles checking the expected cached file checksum
		//   So we do not want to runtime cache it via the service worker
		//
		// NOTE: These named runtime caches are shared across all different build variants of WZ hosted on a domain
		//       So set maxEntries to constrain cache size and evict old entries
		//
		// Cache music & terrain_overrides data loader JS for offline use
		{
			urlPattern: new RegExp('/(music|terrain_overrides)/.*\.js$'),
			handler: 'NetworkFirst',
			options: {
				cacheName: 'optional-data-js-loaders',
				expiration: {
					maxEntries: 12,
					purgeOnQuotaError: true
				}
			}
		},
		// Music & terrain_overrides data
		// Note: Each build will generate music and terrain_override packages, but these are not expected to change frequently.
		//       Since the .data files should be the same (unless Emscripten's file_packager.py changes its packing format),
		//       ideally do not cache them here, and instead rely on Emscripten's preload-cache (which will handle differences if they occur)
		//       (Otherwise we'd end up with likely duplicate data in the cache, if the user loads multiple build variants of WZ hosted on a domain)
		// {
		// 	urlPattern: new RegExp('/(music|terrain_overrides)/.*\.data$'),
		// 	handler: 'NetworkFirst',
		// 	options: {
		// 		cacheName: 'optional-data-packages',
		// 		expiration: {
		// 			maxEntries: 4,
		// 			purgeOnQuotaError: true
		// 		}
		// 	}
		// },
		//
		// Backup on-demand caching of any additional utilized CSS and JS files for offline use
		// (useful in case someone forgot to update the additionalManifestEntries above)
		{
			urlPattern: ({url}) => (url.pathname.endsWith('.js') || url.pathname.endsWith('.css')) && !url.pathname.endsWith('service-worker.js') && url.origin !== 'https://static.cloudflareinsights.com',
			handler: 'NetworkFirst',
			options: {
				cacheName: 'additional-dependencies',
				expiration: {
					maxEntries: 20,
					purgeOnQuotaError: true
				}
			}
		},
	],
	// -----------------------------
	swDest: './service-worker.js',
	offlineGoogleAnalytics: false,
	ignoreURLParametersMatching: [
		/^utm_/,
		/^fbclid$/
	]
};
