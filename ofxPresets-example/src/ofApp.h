#pragma once

#include "ofMain.h"
#include "ofEvent.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyReleased(ofKeyEventArgs& e);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);

		
		//ofEvent<bool> onPresetChanged;
		void onPresetChanged();
};
