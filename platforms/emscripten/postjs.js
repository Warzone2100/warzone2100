
/* Setup persistent config dir */
function wzSetupPersistentConfigDir() {

	Module['WZVAL_configDirPath'] = '';
	if (typeof wz_js_get_config_dir_path === "function") {
		Module['WZVAL_configDirPath'] = wz_js_get_config_dir_path();
	}
	else {
		console.log('Unable to get config dir suffix');
		Module['WZVAL_configDirPath'] = '/warzone2100';
	}

	let configDirPath = Module['WZVAL_configDirPath'];

	// Create a directory in IDBFS to store the config dir
	FS.mkdir(configDirPath);

	// Mount IDBFS as the file system
	FS.mount(FS.filesystems.IDBFS, {}, configDirPath);

	// Synchronize IDBFS -> Emscripten virtual filesystem
	Module["addRunDependency"]("persistent_warzone2100_config_dir");
	FS.syncfs(true, (err) => {
		console.log('loaded from idbfs', FS.readdir(configDirPath));
		Module["removeRunDependency"]("persistent_warzone2100_config_dir");
	})
}
function wzSaveConfigDirToPersistentStore(callback) {
	let configDirPath = Module['WZVAL_configDirPath'];
	FS.syncfs(false, (err) => {
		if (!err) {
			console.log('saved to idbfs', FS.readdir(configDirPath));
		} else {
			console.warn('Failed to save to idbfs store - data may not be persisted');
		}
		if (callback) callback(err);
	})
}
Module.wzSaveConfigDirToPersistentStore = wzSaveConfigDirToPersistentStore;

if (!Module['preRun'])
{
	Module['preRun'] = [];
}
Module['preRun'].push(wzSetupPersistentConfigDir);

Module["onExit"] = function() {
	// Sync IDBFS
	wzSaveConfigDirToPersistentStore(() => {
		if (typeof wz_js_display_loading_indicator === "function") {
			wz_js_display_loading_indicator(false);
		}
		else {
			alert('It is now safe to close your browser window.');
		}
	});

	if (typeof wz_js_handle_app_exit === "function") {
		wz_js_handle_app_exit();
	}
}

Module['ASAN_OPTIONS'] = 'halt_on_error=0'
