#pragma once

#include <glm/vec2.hpp>
#include <vector>

namespace paintcore {

class StrokeSampler {
public:
	void beginStroke(
		const glm::vec2 & position,
		std::vector<glm::vec2> & points);

	// ドラッグで届いた入力座標を追加する
	void addStrokePoint(
		const glm::vec2 & position,
		std::vector<glm::vec2> & points);

private:
	// 2次ベジェ曲線を細かくたどる
	void appendQuadratic(
		const glm::vec2 & start,
		const glm::vec2 & control,
		const glm::vec2 & end,
		std::vector<glm::vec2> & points);

	// 曲線をたどった距離に応じて、描画点を追加する
	void appendPathSample(
		const glm::vec2 & position,
		std::vector<glm::vec2> & points);

	// 直前に届いた、補間前の入力座標
	glm::vec2 previousInputPoint { 0.0f, 0.0f };
	// 次のベジェ曲線の始点
	glm::vec2 curveStartPoint { 0.0f, 0.0f };
	// 曲線を細かくたどる際の直前の座標
	glm::vec2 previousPathSample { 0.0f, 0.0f };
	// 最後に円を置いてから進んだ距離
	float distanceSinceStamp = 0.0f;
};

}
