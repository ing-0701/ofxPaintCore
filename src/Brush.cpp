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
    detailedStamp = {};
	if (newStamp) {
		stamp = std::move(newStamp);
	} else {
		stamp = drawCircleStamp;
	}
}

void Brush::setDetailedStamp(DetailedStamp value) { detailedStamp = std::move(value); }
void Brush::setImageStamp(std::shared_ptr<ofImage> image) {
    if (!image || !image->isAllocated()) { setStamp({}); return; }
    setDetailedStamp([image](const StampInfo& info) {
        ofPushMatrix();
        ofTranslate(info.position.x, info.position.y);
        ofRotateDeg(info.angleDegrees);
        image->draw(-info.radius, -info.radius, info.radius * 2, info.radius * 2);
        ofPopMatrix();
    });
}

// 設定されている色・太さ・不透明度でpointsに沿ってブラシを描画
void Brush::draw(
	const std::vector<glm::vec2>& points,
	std::size_t firstPoint,
	const BrushSettings& settings, std::uint64_t strokeId, std::uint64_t indexOffset) const {
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
        if (detailedStamp) {
            // Stable hash: the same stamp keeps the same angle on every preview redraw.
            std::uint64_t hash = (strokeId * 0x9e3779b97f4a7c15ULL) ^ (indexOffset + i);
            hash ^= hash >> 30; hash *= 0xbf58476d1ce4e5b9ULL;
            hash ^= hash >> 27; hash *= 0x94d049bb133111ebULL;
            hash ^= hash >> 31;
            const float random = static_cast<float>(hash & 0xffffff) / 16777215.0f;
            const float angle = settings.angleDegrees + (random * 2.0f - 1.0f) * settings.randomAngleDegrees;
            detailedStamp({strokeId, indexOffset + i, points[i], radius, angle});
        } else {
            stamp(points[i], radius);
        }
	}

}

}
