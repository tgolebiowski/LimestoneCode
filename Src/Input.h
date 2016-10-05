struct InputState {
	enum SPC_KEY_ENUM {
		CTRL = 0, 
		BACKSPACE = 1, 
		TAB = 2, 
		DEL = 3
	};

	bool romanCharKeys[32];
	bool spcKeys[8];

	char keysPressedSinceLastUpdate [24];

	float mouseX;
	float mouseY;
	bool mouseButtons[4];

	struct {
		float leftStick_x, leftStick_y;
		float rightStick_x, rightStick_y;
		float leftTrigger, rightTrigger;
		bool leftBumper, rightBumper;
		bool button1, button2, button3, button4;
		bool specialButtonLeft, specialButtonRight;
	} controllerState;
};

#ifdef APP_H
static bool IsKeyDown( InputState* i, char key ) {
	if( key >= 'A' && key <= 'Z' ) key += ( 'a' - 'A' );
	return i->romanCharKeys[ key - 'a' ];
}
#endif