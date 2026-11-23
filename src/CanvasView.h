#pragma once
#include "ofMain.h"

namespace paintcore {
// Display/input transformation only; it never owns document images.
class CanvasView {
public:
    glm::vec2 origin { 0, 0 };
    float scale = 1.0f;
    glm::vec2 toCanvas(const glm::vec2& screen) const {
        return (screen - origin) / validScale();
    }
    glm::vec2 toScreen(const glm::vec2& canvas) const {
        return origin + canvas * validScale();
    }
    bool contains(const glm::vec2& screen, int width, int height) const {
        const auto p = toCanvas(screen);
        return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height;
    }
    void begin() const {
        ofPushMatrix();
        ofTranslate(origin.x, origin.y);
        ofScale(validScale(), validScale());
    }
    void end() const { ofPopMatrix(); }
private:
    float validScale() const { return std::isfinite(scale) && scale > 0 ? scale : 1.0f; }
};
}
