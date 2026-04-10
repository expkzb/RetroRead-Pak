#pragma once

#include <string>
#include <vector>

#include "core/BookTypes.h"
#include "platform/Renderer.h"

class DialogueBox {
public:
    void setBounds(const Rect& rect);
    void setTitle(const std::string& title);
    void setBodyLines(const std::vector<std::string>& lines);
    void setBodyRevealTexts(const std::vector<std::string>& revealTexts);
    void setHint(const std::string& hint);
    void render(Renderer& renderer, const ReaderSettings& settings);

private:
    Rect bounds_{40, 420, 1200, 240};
    std::string title_;
    std::vector<std::string> bodyLines_;
    std::vector<std::string> bodyRevealTexts_;
    std::string hint_;
};
