#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "app/Application.h"
#include "epub/EpubCompiler.h"
#include "platform/PlatformFactory.h"
#include "platform/nextui/NextUIBindings.h"

namespace {
int runProbe(const std::string& epubPath) {
    EpubCompiler compiler;
    BookScript book;
    if (!compiler.compile(epubPath, book)) {
        std::cerr << "Failed to compile EPUB: " << epubPath << "\n";
        return 2;
    }

    std::cout << "Title: " << book.title << "\n";
    std::cout << "Author: " << book.author << "\n";
    std::cout << "Chapters: " << book.chapters.size() << "\n";

    const std::size_t previewCount = std::min<std::size_t>(book.chapters.size(), 5);
    for (std::size_t i = 0; i < previewCount; ++i) {
        const Chapter& chapter = book.chapters[i];
        std::cout << "- Chapter " << (i + 1) << ": " << chapter.title
                  << " (" << chapter.sentences.size() << " sentences)\n";
        const std::size_t sentencePreviewCount = std::min<std::size_t>(chapter.sentences.size(), 8);
        for (std::size_t j = 0; j < sentencePreviewCount; ++j) {
            std::cout << "  [" << (j + 1) << "] "
                      << (chapter.sentences[j].isTitle ? "(title) " : "")
                      << chapter.sentences[j].text << "\n";
        }
    }

    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv, argv + argc);
    if (args.size() >= 3 && args[1] == "--probe") {
        return runProbe(args[2]);
    }

    PlatformBackend backend = PlatformBackend::SDL;
    if (args.size() >= 2 && args[1] == "--nextui") {
        backend = PlatformBackend::NextUI;
        installNextUIBindings();
    }

    PlatformServices platform = PlatformFactory::create(backend);

    Application app(
        std::move(platform.renderer),
        std::move(platform.input),
        std::move(platform.fileSystem),
        std::move(platform.clock));

    if (!app.initialize()) {
        return 1;
    }

    app.run();
    app.shutdown();
    return 0;
}
