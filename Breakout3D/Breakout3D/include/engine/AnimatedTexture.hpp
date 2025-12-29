#pragma once

#include "engine/Texture.hpp"
#include <string>
#include <vector>

namespace engine {

// Simple animated texture (GIF) decoded via stb_image and uploaded as per-frame GL textures.
struct AnimatedTexture2D {
    std::vector<Texture2D> frames;
    std::vector<int> delaysMs; // per-frame delay; if empty, assume fixed 100ms

    void destroy();
    bool empty() const { return frames.empty(); }

    // Pick a frame based on elapsed seconds.
    const Texture2D& frameAtTime(float tSec) const;

    // `flipY`: set to true if you want to match Texture2D::loadFromFile(flipY=true) behavior.
    // `maxFrames`: clamp uploaded frames (reduces GPU memory / load time).
    // `frameStep`: upload every Nth frame (reduces GPU memory / load time).
    static AnimatedTexture2D loadGifFromFile(const std::string& path, bool flipY = true, int maxFrames = 120, int frameStep = 2);
};

} // namespace engine


