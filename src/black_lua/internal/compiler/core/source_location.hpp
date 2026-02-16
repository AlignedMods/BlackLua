#pragma once

#include "internal/compiler/core/string_view.hpp"

namespace BlackLua::Internal {

    struct SourceLocation {
        size_t Line = 0;
        size_t Column = 0;

        SourceLocation() = default;
        SourceLocation(size_t line, size_t column)
            : Line(line), Column(column) {}

        inline bool IsValid() const { return Column != 0; }
    };

    struct SourceRange {
        SourceLocation Start;
        SourceLocation End;
        StringView SourceString;

        SourceRange() = default;
        SourceRange(SourceLocation start, SourceLocation end, StringView source)
            : Start(start), End(end), SourceString(source) {}
        SourceRange(size_t startLine, size_t startColumn, size_t endLine, size_t endColumn, StringView source)
            : Start(startLine, startColumn), End(endLine, endColumn), SourceString(source) {}

        inline bool IsValid() const { return Start.IsValid() && End.IsValid() && SourceString.Size() != 0; }
    };

} // namespace BlackLua::Internal