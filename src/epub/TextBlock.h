#pragma once

#include <string>

enum class BlockType {
    Title,
    Paragraph,
    Quote,
    ListItem,
};

struct TextBlock {
    BlockType type = BlockType::Paragraph;
    std::string text;
};
