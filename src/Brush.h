#pragma once

#include "ofMain.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace paintcore {
float brushRadius(float strokeSize);

struct BrushSettings {
	float size = 0.1f;
	ofColor color = ofColor::black;
	bool isPen = true;
	float eraserAlpha = 255.0f;
    float angleDegrees = 0.0f;
    float randomAngleDegrees = 0.0f;
};

struct StampInfo {
    std::uint64_t strokeId;
    std::uint64_t index;
    glm::vec2 position;
    float radius;
    float angleDegrees;
};

class Brush {
public:
	// 1個分のブラシ跡を描く関数。引数は中心座標と半径。
	// ブラシの種類に応じて任意の関数が代入されて使われる
	using Stamp = std::function<void(const glm::vec2&, float)>;

	// 円ブラシで初期化
	Brush();
	void setStamp(Stamp newStamp);
    using DetailedStamp = std::function<void(const StampInfo&)>;
    void setDetailedStamp(DetailedStamp newStamp);
    void setImageStamp(std::shared_ptr<ofImage> image);

	// 設定されている色・太さ・不透明度でpointsに沿ってブラシを描画
	void draw(
		const std::vector<glm::vec2> & points,
		std::size_t firstPoint,
		const BrushSettings & settings,
        std::uint64_t strokeId = 0,
        std::uint64_t indexOffset = 0) const;

private:
	Stamp stamp;
    DetailedStamp detailedStamp;
};

}
