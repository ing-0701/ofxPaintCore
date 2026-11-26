#pragma once

#include "ofMain.h"
#include "ofxGui.h"

namespace paintcoreui {
class ColorPicker {
public:
	void setup(float x, float y, float displaySize = 256.0f);
	void drawPicker();
	void drawHueSlider();

	// 画面座標から色を選ぶ
	// 枠の外では最寄りの端に丸める
	void pick(float x, float y);

	ofColor getColor() const;
	void setEnabled(bool enabled);
    void setColor(const ofColor& color);
    void select(float x, float y);
    void drawSelection() const;
    bool contains(float x, float y) const { return pickerRect.inside(x, y); }
    void update();

private:
	static constexpr int SIZE = 256;

	ofxFloatSlider hue;
	ofPixels pixels;
	ofTexture texture;
	ofRectangle pickerRect;

	float lastHue = -1.0f;
	ofColor pickedColor = ofColor::black;
    glm::vec2 selection { 0, 255 };

	void updateTexture();
	ofColor colorAt(int x, int y);

};
}
