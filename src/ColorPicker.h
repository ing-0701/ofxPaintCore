#pragma once

#include "ofMain.h"
#include "ofxGui.h"

namespace paintcoreui {
class ColorPicker {
public:
	void setup(float x, float y);
	void drawPicker();
	void drawHueSlider();

	// 画面座標から色を選ぶ
	// 枠の外では最寄りの端に丸める
	void pick(float x, float y);

	ofColor getColor() const;

private:
	static constexpr int SIZE = 256;

	ofxFloatSlider hue;
	ofPixels pixels;
	ofTexture texture;
	ofRectangle pickerRect;

	float lastHue = -1.0f;
	ofColor pickedColor = ofColor::black;

	void updateTexture();
	ofColor colorAt(int x, int y);

};
}
