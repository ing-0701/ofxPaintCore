#include "ColorPicker.h"

namespace paintcoreui {

void ColorPicker::setup(float x, float y, float displaySize) {
	// 大きさが0以下になることを防ぐ
	const float size = displaySize >= 1.0f ? displaySize : 1.0f;

	pickerRect.set(x, y, size, size);

	hue.setup("Hue", 0.0f, 0.0f, 1.0f);
	hue.setSize(size, 30);
	hue.setPosition(x, y + size + 2);

	// 色の画像の解像度は、表示サイズに関係なく256×256
	pixels.allocate(SIZE, SIZE, OF_PIXELS_RGBA);
	texture.allocate(pixels);

	lastHue = -1.0f;
	pickedColor = ofColor::black;
	selection = { 0.0f, SIZE - 1.0f };

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
	texture.draw(
		pickerRect.x,
		pickerRect.y,
		pickerRect.width,
		pickerRect.height);
	ofPopStyle();
}

void ColorPicker::drawHueSlider() {
	hue.draw();
}

void ColorPicker::select(float x, float y) {
	// 画面上の位置を、256×256の色画像内の位置へ変換する
	const float textureX = (x - pickerRect.x) * SIZE / pickerRect.width;

	const float textureY = (y - pickerRect.y) * SIZE / pickerRect.height;

	const int localX = static_cast<int>(
		ofClamp(textureX, 0.0f, SIZE - 1.0f));

	const int localY = static_cast<int>(
		ofClamp(textureY, 0.0f, SIZE - 1.0f));

	pickedColor = colorAt(localX, localY);

	selection = {
		static_cast<float>(localX),
		static_cast<float>(localY)
	};
}

void ColorPicker::pick(float x, float y) {
    select(x, y);
    drawSelection();
}

void ColorPicker::drawSelection() const {
	// 色画像内の位置を、画面上の位置へ戻す
	const float localX = selection.x * pickerRect.width / SIZE;

	const float localY = selection.y * pickerRect.height / SIZE;

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

void ColorPicker::setEnabled(bool enabled) {
	if (enabled) {
		hue.registerMouseEvents();
	} else {
		hue.unregisterMouseEvents();
	}
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
