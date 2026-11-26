#pragma once

#include "Brush.h"
#include "ofxGui.h"

namespace paintcoreui {

class BrushControls {
public:
	ofxFloatSlider penSize;
	ofxFloatSlider penAlpha;
	ofxFloatSlider eraserSize;
	ofxFloatSlider eraserAlpha;

	void setup(
		float x,
		float y,
		float width = 256.0f,
		const paintcore::BrushSettings & initial = {},
		float minimumSize = 0.05f,
		float maximumSize = 1.0f,
		float minimumAlpha = 0.01f,
		float maximumAlpha = 1.0f) {
		setupSlider(
			penSize, "penSize",
			initial.size, minimumSize, maximumSize,
			x, y, width);

		setupSlider(
			penAlpha, "penAlpha",
			initial.color.a / 255.0f,
			minimumAlpha, maximumAlpha,
			x, y + 32.0f, width);

		setupSlider(
			eraserSize, "eraserSize",
			initial.eraserSize, minimumSize, maximumSize,
			x, y, width);

		setupSlider(
			eraserAlpha, "eraserAlpha",
			initial.eraserAlpha / 255.0f,
			minimumAlpha, maximumAlpha,
			x, y + 32.0f, width);
	}

	void draw(bool isPen) {
		if (isPen) {
			penSize.draw();
			penAlpha.draw();
		} else {
			eraserSize.draw();
			eraserAlpha.draw();
		}
	}

	// 色のRGB、ペン／消しゴムの選択、配置間隔などは変更しない。
	void applyTo(paintcore::BrushSettings & settings) {
		settings.size = static_cast<float>(penSize);
		settings.eraserSize = static_cast<float>(eraserSize);

		settings.color.a = static_cast<unsigned char>(
			static_cast<float>(penAlpha) * 255.0f);

		settings.eraserAlpha = static_cast<float>(eraserAlpha) * 255.0f;
	}

private:
	void setupSlider(
		ofxFloatSlider & slider,
		const std::string & name,
		float initial,
		float minimum,
		float maximum,
		float x,
		float y,
		float width) {
		slider.setup(
			name,
			ofClamp(initial, minimum, maximum),
			minimum,
			maximum);

		slider.setSize(width, 30.0f);
		slider.setPosition(x, y);
	}
};

}
