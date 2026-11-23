#include "ColorPicker.h"

namespace paintcoreui {

void ColorPicker::setup(float x, float y) {
	pickerRect.set(x, y, SIZE, SIZE);

	hue.setup("Hue", 0.0f, 0.0f, 1.0f);
	hue.setSize(SIZE, 30);
	hue.setPosition(x, y + SIZE + 2);

	pixels.allocate(SIZE, SIZE, OF_PIXELS_RGBA);
	texture.allocate(pixels);

	lastHue = -1.0f;
	pickedColor = ofColor::black;
	updateTexture();
}

ofColor ColorPicker::colorAt(int x, int y) {
	ofColor color;
	color.setHsb(
		static_cast<float>(hue) * 255.0f,
		x,
		255 - y);
	return color;
}

void ColorPicker::updateTexture() {
	const float currentHue = hue;

	if (currentHue == lastHue) {
		return;
	}

	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			pixels.setColor(x, y, colorAt(x, y));
		}
	}

	texture.loadData(pixels);
	lastHue = currentHue;
}

void ColorPicker::drawPicker() {
	updateTexture();

	ofPushStyle();
	ofSetColor(255);
	texture.draw(pickerRect.x, pickerRect.y, SIZE, SIZE);
	ofPopStyle();
}

void ColorPicker::drawHueSlider() {
	hue.draw();
}

void ColorPicker::select(float x, float y) {
	const int localX = static_cast<int>(
		ofClamp(x - pickerRect.x, 0.0f, SIZE - 1.0f));
	const int localY = static_cast<int>(
		ofClamp(y - pickerRect.y, 0.0f, SIZE - 1.0f));

	pickedColor = colorAt(localX, localY);
    selection = { localX, localY };
}

void ColorPicker::pick(float x, float y) {
    select(x, y);
    drawSelection();
}

void ColorPicker::drawSelection() const {
    const float localX = selection.x, localY = selection.y;

	// 選択位置に、現在の色を示す円を表示する
	ofPushStyle();

	ofFill();
	ofSetColor(pickedColor);
	ofDrawCircle(
		pickerRect.x + localX,
		pickerRect.y + localY,
		10);

	ofNoFill();
	ofSetColor(150);
	ofSetLineWidth(2);
	ofDrawCircle(
		pickerRect.x + localX,
		pickerRect.y + localY,
		10);

	ofPopStyle();
}

ofColor ColorPicker::getColor() const {
	return pickedColor;
}

void ColorPicker::setColor(const ofColor& color) {
    pickedColor = color;
    hue = color.getHue() / 255.0f;
    selection = {color.getSaturation(), 255.0f - color.getBrightness()};
    updateTexture();
}
void ColorPicker::update() {
    if (static_cast<float>(hue) != lastHue) {
        updateTexture();
        pickedColor = colorAt(static_cast<int>(selection.x), static_cast<int>(selection.y));
    }
}

}
