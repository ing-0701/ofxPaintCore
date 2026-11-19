#include "Brush.h"

#include <utility>

namespace paintcore {

float brushRadius(float strokeSize) {
	return strokeSize * 25.0f;
}

namespace {

void drawCircleStamp(const glm::vec2& center, float radius) {
	ofDrawCircle(center.x, center.y, radius);
}

}

// 円ブラシで初期化
Brush::Brush()
	: stamp(drawCircleStamp) {
}

void Brush::setStamp(Stamp newStamp) {
	if (newStamp) {
		stamp = std::move(newStamp);
	} else {
		stamp = drawCircleStamp;
	}
}

// 設定されている色・太さ・不透明度でpointsに沿ってブラシを描画
void Brush::draw(
	const std::vector<glm::vec2>& points,
	std::size_t firstPoint,
	const BrushSettings& settings) const {
	if (settings.isPen) {
		ofSetColor(settings.color);
		glBlendFuncSeparate(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	} else {
		ofSetColor(255, 255, 255, settings.eraserAlpha);
		glBlendFuncSeparate(
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA,
			GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
	}
	const float radius = brushRadius(settings.size);

	for (std::size_t i = firstPoint; i < points.size(); i++) {
		stamp(points[i], radius);
	}

}

}
