#pragma once

#include "ofMain.h"

#include <cstddef>
#include <functional>
#include <vector>

namespace paintcore {
float brushRadius(float strokeSize);

struct BrushSettings {
	float size;
	ofColor color;
	bool isPen;
	float eraserAlpha;
};

class Brush {
public:
	// 1個分のブラシ跡を描く関数。引数は中心座標と半径。
	// ブラシの種類に応じて任意の関数が代入されて使われる
	using Stamp = std::function<void(const glm::vec2&, float)>;

	// 円ブラシで初期化
	Brush();
	void setStamp(Stamp newStamp);

	// 設定されている色・太さ・不透明度でpointsに沿ってブラシを描画
	void draw(
		const std::vector<glm::vec2> & points,
		std::size_t firstPoint,
		const BrushSettings & settings) const;

private:
	Stamp stamp;
};

}
