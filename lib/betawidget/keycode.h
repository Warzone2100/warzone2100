#ifndef KEYCODE_H_
#define KEYCODE_H_

/**
 * Contains the list of known keycodes for keyboard input.
 */
typedef enum
{
	// `Control' keys
	EVT_KEYCODE_BACKSPACE,
	EVT_KEYCODE_TAB,
	EVT_KEYCODE_RETURN,
	EVT_KEYCODE_ESCAPE,
	EVT_KEYCODE_ENTER,
	
	// Arrow keys
	EVT_KEYCODE_LEFT,
	EVT_KEYCODE_RIGHT,
	EVT_KEYCODE_UP,
	EVT_KEYCODE_DOWN,
	
	// Home/end pad
	EVT_KEYCODE_INSERT,
	EVT_KEYCODE_DELETE,
	EVT_KEYCODE_HOME,
	EVT_KEYCODE_END,
	EVT_KEYCODE_PAGEUP,
	EVT_KEYCODE_PAGEDOWN,
	
	// Unknown
	EVT_KEYCODE_UNKNOWN
} eventKeycode;

#endif /*KEYCODE_H_*/
