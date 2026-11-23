#include "PaintSession.h"
#include <cstring>
#include <limits>

namespace paintcore {
// CBOR stores layer pixels as binary IEEE754 float32 (little endian), not 8-bit PNG.
// It therefore retains premultiplied alpha, including values outside [0, 1].
bool PaintSession::saveProject(const std::string& path) {
    if (!initialized || isDrawing()) return false;
    try {
        ofJson doc = {{"format", "PaintCore"}, {"version", 1},
            {"width", canvasWidth}, {"height", canvasHeight},
            {"selected", selectedLayerId}, {"nextId", nextLayerId},
            {"background", {background.r, background.g, background.b, background.a}},
            {"layers", ofJson::array()}};
        for (const auto& layer : engine.getLayers()) {
            ofFloatPixels pixels;
            layer->canvas.readToPixels(pixels);
            if (pixels.getNumChannels() != 4) return false;
            std::vector<std::uint8_t> bytes(pixels.size() * 4);
            for (std::size_t i = 0; i < pixels.size(); ++i) {
                std::uint32_t bits;
                std::memcpy(&bits, &pixels[i], 4);
                for (int b = 0; b < 4; ++b) bytes[i*4+b] = (bits >> (b*8)) & 255;
            }
            doc["layers"].push_back({{"id", layer->id}, {"name", layer->name},
                {"visible", layer->visible}, {"locked", layer->locked}, {"opacity", layer->opacity},
                {"position", {layer->position.x, layer->position.y}},
                {"extension", getExtension(layer->id).saveState()},
                {"pixels", ofJson::binary(bytes)}});
        }
        const auto bytes = ofJson::to_cbor(doc);
        const auto destination = of::filesystem::path(ofToDataPath(path, true));
        auto temporary = destination;
        temporary += ".paintcore-tmp";
        if (!ofBufferToFile(temporary, ofBuffer(reinterpret_cast<const char*>(bytes.data()), bytes.size()), true)) return false;
        std::error_code error;
        of::filesystem::rename(temporary, destination, error);
        if (error) { of::filesystem::remove(temporary); return false; }
        modified = false;
        return true;
    } catch (const std::exception& error) {
        ofLogError("PaintCore saveProject") << error.what();
        return false;
    }
}

bool PaintSession::loadProject(const std::string& path, const ExtensionFactory& factory) {
    if (!initialized || isDrawing()) return false;
    try {
        const ofBuffer data = ofBufferFromFile(path, true);
        if (data.size() == 0) return false;
        const auto* first = reinterpret_cast<const std::uint8_t*>(data.getData());
        const ofJson doc = ofJson::from_cbor(first, first + data.size());
        if (doc.at("format") != "PaintCore" || doc.at("version") != 1) return false;
        // This session keeps its allocated canvas size/history buffers.
        // Reject mismatches instead of silently scaling the user's work.
        if (doc.at("width") != canvasWidth || doc.at("height") != canvasHeight) return false;
        const auto& items = doc.at("layers");
        if (!items.is_array() || items.empty()) return false;
        const std::size_t floatCount = static_cast<std::size_t>(canvasWidth) * canvasHeight * 4;
        std::vector<std::unique_ptr<Layer>> loaded;
        std::unordered_map<int, std::unique_ptr<LayerExtension>> loadedExtensions;
        int highestId = 0;
        for (const auto& item : items) {
            const int id = item.at("id").get<int>();
            if (id <= 0 || loadedExtensions.count(id)) return false;
            const auto& bytes = item.at("pixels").get_binary();
            if (bytes.size() != floatCount * 4) return false;
            ofFloatPixels pixels;
            pixels.allocate(canvasWidth, canvasHeight, OF_PIXELS_RGBA);
            for (std::size_t i = 0; i < floatCount; ++i) {
                std::uint32_t bits = 0;
                for (int b = 0; b < 4; ++b) bits |= static_cast<std::uint32_t>(bytes[i*4+b]) << (b*8);
                std::memcpy(&pixels[i], &bits, 4);
                if (!std::isfinite(pixels[i])) return false;
            }
            auto layer = std::make_unique<Layer>(id, item.at("name").get<std::string>(), paintSettings);
            if (!layer->canvas.isAllocated()) return false;
            layer->visible = item.at("visible").get<bool>();
            layer->locked = item.at("locked").get<bool>();
            layer->opacity = item.at("opacity").get<float>();
            layer->position = {item.at("position").at(0).get<float>(), item.at("position").at(1).get<float>()};
            if (!std::isfinite(layer->opacity) || layer->opacity < 0 || layer->opacity > 1
                || !std::isfinite(layer->position.x) || !std::isfinite(layer->position.y)) return false;
            layer->canvas.getTexture().loadData(pixels);
            const auto& state = item.at("extension");
            std::unique_ptr<LayerExtension> extension;
            if (factory) extension = factory(state);
            else if (state.at("type") == "Normal") extension = std::make_unique<LayerExtension>();
            if (!extension) return false; // Never silently drop an unknown app effect.
            extension->setup(canvasWidth, canvasHeight);
            extension->markDirty();
            loadedExtensions.emplace(id, std::move(extension));
            loaded.push_back(std::move(layer));
            highestId = std::max(highestId, id);
        }
        const int selected = doc.at("selected").get<int>();
        const int nextId = doc.at("nextId").get<int>();
        if (!loadedExtensions.count(selected) || nextId <= highestId) return false;
        const auto& bg = doc.at("background");
        ofColor newBackground(bg.at(0).get<unsigned char>(), bg.at(1).get<unsigned char>(),
            bg.at(2).get<unsigned char>(), bg.at(3).get<unsigned char>());
        // Everything above succeeded. Only now replace the current document/history.
        clearHistory();
        while (!engine.getLayers().empty()) engine.takeLayerAt(0);
        for (auto& layer : loaded) engine.insertLayerAt(static_cast<int>(engine.getLayers().size()), std::move(layer));
        extensions = std::move(loadedExtensions);
        selectedLayerId = selected;
        nextLayerId = nextId;
        background = newBackground;
        modified = false;
        return true;
    } catch (const std::exception& error) {
        ofLogError("PaintCore loadProject") << error.what();
        return false;
    }
}

bool PaintSession::exportPng(const std::string& path, bool transparent) {
    if (!initialized || isDrawing()) return false;
    if (transparent) {
        for (const auto& layer : engine.getLayers())
            if (layer->visible && layer->opacity > 0 && !getExtension(layer->id).supportsTransparentExport()) return false;
    }
    ofFbo output;
    output.allocate(paintSettings.makeFboSettings());
    if (!output.isAllocated()) return false;
    ofPushStyle();
    output.begin();
    ofClear(0,0,0,0);
    if (transparent) drawCanvas(false); else draw();
    output.end();
    ofPopStyle();
    ofFloatPixels source;
    output.readToPixels(source);
    ofPixels image;
    image.allocate(canvasWidth, canvasHeight, OF_PIXELS_RGBA);
    for (std::size_t i = 0; i < source.size(); i += 4) {
        const float a = ofClamp(source[i+3], 0.0f, 1.0f);
        // PNG stores straight alpha; our canvas stores premultiplied alpha.
        for (int c = 0; c < 3; ++c)
            image[i+c] = static_cast<unsigned char>(std::round(ofClamp(a > 0 ? source[i+c]/a : 0, 0.0f, 1.0f)*255));
        image[i+3] = static_cast<unsigned char>(std::round(a*255));
    }
    return ofSaveImage(image, path);
}
}
