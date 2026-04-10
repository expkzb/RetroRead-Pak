#pragma once

#include <vector>

#include "core/BookTypes.h"
#include "ui/AppScene.h"

class BookListScene : public AppScene {
public:
    explicit BookListScene(Application& app);

    void onEnter() override;
    void update(float dt) override;
    void render(Renderer& renderer) override;

private:
    struct BookListEntry {
        BookListItem item;
        bool hasCache = false;
        bool hasProgress = false;
        ReadingProgress progress;
        std::string author;
        std::size_t chapterCount = 0;
        std::size_t totalSentences = 0;
        int percentRead = 0;
    };

    BookScript makeDebugBook() const;
    std::string buildListStatus(const BookListEntry& entry) const;
    int visibleEntryCount() const;
    void clampScroll();

    std::vector<BookListEntry> books_;
    int selectedIndex_ = 0;
    int scrollOffset_ = 0;
};
