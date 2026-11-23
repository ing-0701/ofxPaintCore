#pragma once

#include "CanvasRenderer.h"

namespace paintcore {
// Optional app-owned behavior. A Layer itself never owns or depends on this.
// The session keeps this object with its layer when undo temporarily removes it.
class LayerExtension {
public:
    virtual ~LayerExtension() = default;
    virtual void setup(int width, int height) {}
    virtual void markDirty() {}
    virtual ofJson saveState() const { return {{"type", "Normal"}}; }
    // Additive light cannot in general be represented by a straight-alpha PNG.
    virtual bool supportsTransparentExport() const { return true; }
    virtual void draw(ofFbo& source, float time) {
        CanvasRenderer::drawNormalImage(source);
    }
};
}
