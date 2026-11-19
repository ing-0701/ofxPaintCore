#include "StrokeSampler.h"

#include <cmath>
#include <glm/geometric.hpp>

namespace paintcore {
void StrokeSampler::beginStroke(
	const glm::vec2 & position,
	std::vector<glm::vec2> & points,
	float spacingPx) {

	spacing = std::isfinite(spacingPx) && spacingPx > 0.0f
		? spacingPx
		: 3.0f;

	previousInputPoint = position;
	curveStartPoint = position;
	previousPathSample = position;
	distanceSinceStamp = 0.0f;

	// クリックだけでも、押した場所に最初のスタンプを置く
	points.push_back(position);
}

void StrokeSampler::appendPathSample(
	const glm::vec2 & position,
	std::vector<glm::vec2>& points) {
	const glm::vec2 delta = position - previousPathSample;
	const float length = glm::length(delta);

	if (length <= 0.0001f) {
		return;
	}

	const glm::vec2 direction = delta / length;

	// この小区間の始点から、次の円を置く位置までの距離
	float nextDistance = spacing - distanceSinceStamp;
	// distanceSinceStampは指定間隔を超えるまでこれまでの距離が蓄積する
	// 例えば指定間隔が3.0pxで、過去の三回のステップがそれぞれ0.7px, 0.8px, 0.8px だったならdistanceSinceStampは2.3

	while (nextDistance <= length) {
		points.push_back(
			previousPathSample + direction * nextDistance);

		nextDistance += spacing;
	}

	// 次の小区間へ持ち越す距離
	distanceSinceStamp = length - (nextDistance - spacing);
	previousPathSample = position;
}

void StrokeSampler::appendQuadratic(
	const glm::vec2 & start,
	const glm::vec2 & control,
	const glm::vec2 & end,
	std::vector<glm::vec2>& points) {

	const float firstLength = glm::distance(start, control);
	const float secondLength = glm::distance(control, end);

	// 視点→制御点と、制御点→終点の二本のうち長いほう
	const float longestLength = firstLength > secondLength ? firstLength : secondLength;

	// ベジェ曲線の式の定義上、t=0からt=1間のどの場所でも速度は2*longestLengthを超えない
	// 曲線の長さは曲線の速度をt=0~1で積分したら求められる
	// その結果長さの上限が2*longestLengthだと求められる
	// よって、曲線の移動ステップを2*longestLengthにすれば、一度のステップでの移動量が1を超えることはない
	// 曲線を約1px以下の小区間に分けてたどる
	int steps = static_cast<int>(
		std::ceil(2.0f * longestLength));

	if (steps < 1) {
		steps = 1;
	}

	for (int i = 1; i <= steps; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(steps);

		const float u = 1.0f - t;

		// ベジェ曲線の式に基づいたステップごとの点の位置の計算
		const glm::vec2 position = u * u * start
			+ 2.0f * u * t * control
			+ t * t * end;

		// 曲線に沿った距離が3pxごとになるよう座標を追加する
		appendPathSample(position, points);
	}
}

void StrokeSampler::addStrokePoint(
	const glm::vec2 & position,
	std::vector<glm::vec2>& points) {
	// 同じ位置が繰り返し届いた場合は、何もしない
	if (glm::distance(position, previousInputPoint) <= 0.0001f) {
		return;
	}

	const glm::vec2 midpoint = (previousInputPoint + position) * 0.5f;

	appendQuadratic(
		curveStartPoint,
		previousInputPoint,
		midpoint,
		points);

	curveStartPoint = midpoint;
	previousInputPoint = position;
}

}
