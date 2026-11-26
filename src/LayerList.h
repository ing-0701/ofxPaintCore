#pragma once

#include "ofMain.h"
#include <algorithm>

namespace paintcoreui {

struct LayerMoveRequest {
	int layerId = 0;
	int destinationIndex = -1;
};

class LayerList {
public:
	void setup(
		float x,
		float y,
		float width,
		float rowHeight,
		int visibleRows) {

		rowHeight_ = std::max(1.0f, rowHeight);
		visibleRows_ = std::max(1, visibleRows);

		rect_.set(
			x,
			y,
			std::max(1.0f, width),
			rowHeight_ * visibleRows_);

		firstVisible_ = 0;
		draggedLayerId_ = 0;
	}

	const ofRectangle & bounds() const {
		return rect_;
	}

	ofRectangle rowRect(int visibleRow) const {
		return ofRectangle(
			rect_.x,
			rect_.y + visibleRow * rowHeight_,
			rect_.width,
			rowHeight_);
	}

	int firstVisibleIndex() const {
		return firstVisible_;
	}

	void clampScroll(int layerCount) {
		const int maximum = std::max(0, layerCount - visibleRows_);

		firstVisible_ = std::clamp(firstVisible_, 0, maximum);
	}

	void scroll(float scrollY, int layerCount) {
		if (scrollY > 0) {
			--firstVisible_;
		} else if (scrollY < 0) {
			++firstVisible_;
		}

		clampScroll(layerCount);
	}

	void beginDrag(int layerId) {
		draggedLayerId_ = layerId;
	}

	void cancelDrag() {
		draggedLayerId_ = 0;
	}

	LayerMoveRequest endDrag(
		float x,
		float y,
		int layerCount) {

		LayerMoveRequest request;

		const int layerId = draggedLayerId_;
		draggedLayerId_ = 0;

		if (layerId == 0 || !rect_.inside(x, y)) {
			return request;
		}

		clampScroll(layerCount);

		const int row = static_cast<int>(
			(y - rect_.y) / rowHeight_);

		const int destination = firstVisible_ + row;

		// 一覧の外や、レイヤーが存在しない空白には移動しない
		if (row < 0 || row >= visibleRows_
			|| destination < 0 || destination >= layerCount) {
			return request;
		}

		request.layerId = layerId;
		request.destinationIndex = destination;
		return request;
	}

private:
	ofRectangle rect_;
	float rowHeight_ = 46.0f;
	int visibleRows_ = 4;
	int firstVisible_ = 0;
	int draggedLayerId_ = 0;
};

}
