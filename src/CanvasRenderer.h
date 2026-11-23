#pragma once

#include "PaintEngine.h"

#include <functional>

namespace paintcore {

class CanvasRenderer {
public:
	// 返り値がvoid、引数がLayer&, ofFbo&の任意の関数
	// ペイントソフトに応じた描画方式をペイントソフト側で指定できる
	// あくまで描画方式だけで、描画の手順はpaintcoreによって決められている
	using DrawLayer = std::function<void(const Layer &, ofFbo &)>;
    using LayerPlacement = std::function<glm::vec2(const Layer&)>;

	static void draw(
		PaintEngine & engine,
		int previewLayerId,
		const DrawLayer & drawLayer = drawNormalLayer,
        const LayerPlacement& placement = {});

	// ペイントソフト側で描画方式を指定しなかった場合のデフォルト描画方式
	static void drawNormalLayer(const Layer & layer, ofFbo & source);
	static void drawNormalImage(ofFbo & source);
};

}
