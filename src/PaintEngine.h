#pragma once

#include "Brush.h"
#include "StrokeSampler.h"
#include "PaintLayer.h"

#include <memory>
#include <cstddef>
#include <vector>

namespace paintcore {

	void copyFbo(ofFbo & source, ofFbo & destination);

class PaintEngine {
public:
	Brush brush;

	void allocateStrokeBuffers(const ofFbo::Settings & settings);

	void beginStroke(const glm::vec2 & position, ofFbo & canvas);
	void addStrokePoint(const glm::vec2 & position);

	void drawStroke(const BrushSettings & settings);
	ofFbo & getStrokePreview();
	void commitStroke(ofFbo & canvas);

	const std::vector<std::unique_ptr<Layer>> & getLayers() const;

	int findLayerIndex(int layerId) const;
	Layer & getLayer(int layerId);

	void insertLayerAt(int index, std::unique_ptr<Layer> layer);
	std::unique_ptr<Layer> takeLayerAt(int index);
	void moveLayer(int layerId, int destinationIndex);

private:
	// この数の未保存点がたまったら途中保存する
	static constexpr std::size_t STROKE_CHUNK_SIZE = 10;

	StrokeSampler strokeSampler;
	std::vector<glm::vec2> points;

	// pointsのうち、描画を始める位置
	// 0：先頭から描く
	// 1：先頭は接続用の点なので、描かずに飛ばす
	std::size_t strokePointStartIndex = 0;

	// 最新の部分が描かれる方
	ofFbo preFbo2D;
	// 描画前のキャンバス全体と、描画中の線の古い部分を保持
	ofFbo strokeCheckpointFbo;

	void clearStroke();
	void drawPendingPoints(const BrushSettings & settings) const;
	bool needsCheckpoints() const;
	void checkpointSaved();
	std::vector<std::unique_ptr<Layer>> layers;
};

}
