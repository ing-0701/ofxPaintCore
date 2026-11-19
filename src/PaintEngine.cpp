#include "PaintEngine.h"

#include <stdexcept>
#include <utility>

namespace paintcore {
void PaintEngine::allocateStrokeBuffers(const ofFbo::Settings & settings) {
	preFbo2D.allocate(settings);
	strokeCheckpointFbo.allocate(settings);
}

void copyFbo(ofFbo& source, ofFbo& destination) {
	ofPushStyle();

	destination.begin();
	ofClear(0, 0, 0, 0);
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);
	source.draw(0, 0);
	destination.end();

	ofPopStyle();
}

void PaintEngine::clearStroke() {
	points.clear();
	strokePointStartIndex = 0;
}

void PaintEngine::beginStroke(const glm::vec2 & position, ofFbo& canvas, float spacingPx) {
	clearStroke();
	strokeSampler.beginStroke(position, points, spacingPx);
	// チェックポイントにレイヤー全体を保存
	// 描画中の線がある程度たまるたびにチェックポイントに追加
	//描画中の線自体はpreFbo2Dに描かれている
	copyFbo(canvas, strokeCheckpointFbo);
}

void PaintEngine::addStrokePoint(const glm::vec2 & position) {
	strokeSampler.addStrokePoint(position, points);
}

void PaintEngine::drawPendingPoints(const BrushSettings & settings) const {
	brush.draw(points, strokePointStartIndex, settings);
}

void PaintEngine::drawStroke(const BrushSettings& settings) {
	ofFill();

	preFbo2D.begin();
	ofClear(0, 0, 0, 0);

	// 途中保存した画像を土台にする
	ofDisableAlphaBlending();
	ofSetColor(255, 255, 255, 255);
	strokeCheckpointFbo.draw(0, 0);

	// 土台の上に未保存のブラシテクスチャを描く
	ofEnableAlphaBlending();
	drawPendingPoints(settings);

	preFbo2D.end();

	if (needsCheckpoints()) {
		copyFbo(preFbo2D, strokeCheckpointFbo);
		checkpointSaved();
	}
}

ofFbo& PaintEngine::getStrokePreview() {
	return preFbo2D;
}

void PaintEngine::commitStroke(ofFbo& canvas) {
	copyFbo(preFbo2D, canvas);
	clearStroke();
}

bool PaintEngine::needsCheckpoints() const {
	return points.size() - strokePointStartIndex >= STROKE_CHUNK_SIZE;
}

void PaintEngine::checkpointSaved() {
	// 次の点との補間に使うため、最後の座標だけ残す
	const glm::vec2 lastPoint = points.back();
	points.clear();
	points.push_back(lastPoint);
	// 残した点はすでに画像へ描画済みなので、再描画しない
	strokePointStartIndex = 1;
}

const std::vector<std::unique_ptr<Layer>>& PaintEngine::getLayers() const {
	return layers;
}

int PaintEngine::findLayerIndex(int layerId) const {
	for (int i = 0; i < static_cast<int>(layers.size()); i++) {
		if (layers[i]->id == layerId) {
			return i;
		}
	}
	return -1;
}

Layer& PaintEngine::getLayer(int layerId) {
	const int index = findLayerIndex(layerId);
	if (index == -1) {
		throw std::out_of_range("Layer ID was not found.");
	}
	return *layers[index];
}

void PaintEngine::insertLayerAt(int index, std::unique_ptr<Layer> layer) {
	if (index < 0 || index > static_cast<int>(layers.size())) {
		throw std::out_of_range("Layer insertion index is out of range.");
	}

	layers.insert(layers.begin() + index, std::move(layer));
}

std::unique_ptr<Layer> PaintEngine::takeLayerAt(int index) {
	if (index < 0 || index >= static_cast<int>(layers.size())) {
		throw std::out_of_range("Layer index is out of range.");
	}

	auto layer = std::move(layers[index]);
	layers.erase(layers.begin() + index);

	return layer;
}

void PaintEngine::moveLayer(int layerId, int destinationIndex) {
	const int count = static_cast<int>(layers.size());
	if (destinationIndex < 0 || destinationIndex >= count) {
		return;
	}

	const int sourceIndex = findLayerIndex(layerId);
	if (sourceIndex == -1 || sourceIndex == destinationIndex) {
		return;
	}

	auto layer = takeLayerAt(sourceIndex);
	insertLayerAt(destinationIndex, std::move(layer));
}

}
