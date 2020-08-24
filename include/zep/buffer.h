#pragma once

#include <functional>
#include <set>

#include "zep/mcommon/file/path.h"
#include "zep/mcommon/utf8/unchecked.h"
#include "zep/mcommon/string/stringutils.h"
#include "zep/mcommon/logger.h"

#include "zep/glyph_iterator.h"

#include "zep/editor.h"
#include "zep/line_widgets.h"
#include "zep/theme.h"

namespace Zep
{

class ZepSyntax;
class ZepTheme;
class ZepMode;
enum class ThemeColor;

enum class SearchDirection
{
    Forward,
    Backward
};

namespace SearchType
{
enum : uint32_t
{
    WORD = (1 << 0),
    Begin = (1 << 1),
    End = (1 << 2),
    Word = (1 << 3),
    SingleLine = (1 << 4)
};
};

namespace FileFlags
{
enum : uint32_t
{
    StrippedCR = (1 << 0),
    TerminatedWithZero = (1 << 1),
    ReadOnly = (1 << 2),
    Locked = (1 << 3), // Can this file path ever be written to?
    Dirty = (1 << 4), // Has the file been changed?
    HasWarnings = (1 << 6),
    HasErrors = (1 << 7),
    DefaultBuffer = (1 << 8), // Default startup buffer
    HasTabs = (1 << 9),
    HasSpaceTabs = (1 << 10),
    InsertTabs = (1 << 11)
};
}

// Ensure the character is >=0 and <=127 as in the ASCII standard,
// isalnum, for example will assert on debug build if not in this range.
inline int ToASCII(const char ch)
{
    auto ret = (unsigned int)ch;
    ret = std::min(ret, 127u);
    return ret;
}

enum class BufferType
{
    Normal,
    Search,
    Repl,
    DataGrid,
    Tree
};

namespace RangeMarkerDisplayType
{
enum
{
    Hidden = 0,
    Underline = (1 << 0), // Underline the range
    Background = (1 << 1), // Add a background to the range
    Tooltip = (1 << 2), // Show a tooltip using the name/description
    TooltipAtLine = (1 << 3), // Tooltip shown if the user hovers the line
    CursorTip = (1 << 4), // Tooltip shown if the user cursor is on the Mark
    CursorTipAtLine = (1 << 5), // Tooltip shown if the user cursor is on the Mark line
    Indicator = (1 << 6), // Show an indicator on the left side
    All = Underline | Tooltip | TooltipAtLine | CursorTip | CursorTipAtLine | Indicator | Background
};
};

enum class ToolTipPos
{
    AboveLine = 0,
    BelowLine = 1,
    RightLine = 2,
    Count = 3
};

namespace RangeMarkerType
{
enum
{
    Message = (1 << 0),
    Search = (1 << 1),
    All = (Message | Search)
};
};

using ByteIndex = long;
struct ByteRange
{
    ByteRange(ByteIndex a = 0, ByteIndex b = 0)
        : first(a),
        second(b)
    { }
    ByteIndex first;
    ByteIndex second;
    bool ContainsLocation(ByteIndex loc) const
    {
        return loc >= first && loc < second;
    }
};

struct RangeMarker
{
    ByteRange range;
    ThemeColor textColor = ThemeColor::Text;
    ThemeColor backgroundColor = ThemeColor::Background;
    ThemeColor highlightColor = ThemeColor::Background;
    uint32_t displayType = RangeMarkerDisplayType::All;
    uint32_t markerType = RangeMarkerType::Message;
    std::string name;
    std::string description;
    ToolTipPos tipPos = ToolTipPos::AboveLine;

    bool ContainsLocation(GlyphIterator loc) const
    {
        return range.ContainsLocation(loc.Index());
    }
    bool IntersectsRange(const ByteRange& i) const
    {
        return i.first < range.second && i.second > range.first;
    }
};

using tRangeMarkers = std::map<ByteIndex, std::set<std::shared_ptr<RangeMarker>>>;

// A really big cursor move; which will likely clamp
//static const GlyphIterator MaxCursorMove = GlyphIterator(0xFFFFFFF);
//const long InvalidByteIndex = -1;

enum class ExpressionType
{
    Inner,
    Outer
};


class ZepBuffer : public ZepComponent
{
public:
    ZepBuffer(ZepEditor& editor, const std::string& strName);
    ZepBuffer(ZepEditor& editor, const ZepPath& path);
    virtual ~ZepBuffer();

    void Clear();
    void SetText(const std::string& strText, bool initFromFile = false);
    void Load(const ZepPath& path);
    bool Save(int64_t& size);

    ZepPath GetFilePath() const;
    std::string GetFileExtension() const;
    void SetFilePath(const ZepPath& path);

    GlyphIterator GetLinePos(GlyphIterator bufferLocation, LineLocation lineLocation) const;
    bool GetLineOffsets(const long line, ByteRange& range) const;
    GlyphIterator ClampToVisibleLine(GlyphIterator in) const;
    long GetBufferColumn(GlyphIterator location) const;
    using fnMatch = std::function<bool(const char)>;

    bool Move(GlyphIterator& loc, SearchDirection dir) const;
    void MotionBegin(GlyphIterator& start) const;
    bool Skip(fnMatch IsToken, GlyphIterator& start, SearchDirection dir) const;
    bool SkipOne(fnMatch IsToken, GlyphIterator& start, SearchDirection dir) const;
    bool SkipNot(fnMatch IsToken, GlyphIterator& start, SearchDirection dir) const;

    GlyphIterator Find(GlyphIterator start, const uint8_t* pBegin, const uint8_t* pEnd) const;
    GlyphIterator FindOnLineMotion(GlyphIterator start, const uint8_t* pCh, SearchDirection dir) const;
    GlyphIterator WordMotion(GlyphIterator start, uint32_t searchType, SearchDirection dir) const;
    GlyphIterator EndWordMotion(GlyphIterator start, uint32_t searchType, SearchDirection dir) const;
    GlyphIterator ChangeWordMotion(GlyphIterator start, uint32_t searchType, SearchDirection dir) const;
    GlyphRange AWordMotion(GlyphIterator start, uint32_t searchType) const;
    GlyphRange InnerWordMotion(GlyphIterator start, uint32_t searchType) const;
    GlyphRange StandardCtrlMotion(GlyphIterator cursor, SearchDirection searchDir) const;

    bool Delete(const GlyphIterator& startOffset, const GlyphIterator& endOffset);
    bool Insert(const GlyphIterator& startOffset, const std::string& str);
    bool Replace(const GlyphIterator& startOffset, const GlyphIterator& endOffset, const std::string& str);

    long GetLineCount() const
    {
        return long(m_lineEnds.size());
    }
    long GetBufferLine(GlyphIterator offset) const;

    GlyphIterator End() const;
    GlyphIterator Begin() const;

    const GapBuffer<uint8_t>& GetGapBuffer() const
    {
        return m_gapBuffer;
    }
    GapBuffer<uint8_t>& GetMutableText()
    {
        return m_gapBuffer;
    }
    const std::vector<ByteIndex> GetLineEnds() const
    {
        return m_lineEnds;
    }

    void SetSyntax(std::shared_ptr<ZepSyntax> syntax)
    {
        m_spSyntax = syntax;
    }

    void SetSyntaxProvider(SyntaxProvider provider)
    {
        if (provider.syntaxID != m_syntaxProvider.syntaxID)
        {
            m_spSyntax = provider.factory(this);
            m_syntaxProvider = provider;
        }
    }

    ZepSyntax* GetSyntax() const
    {
        return m_spSyntax.get();
    }

    const std::string& GetName() const
    {
        return m_strName;
    }

    std::string GetDisplayName() const;
    virtual void Notify(std::shared_ptr<ZepMessage> message) override;

    ZepTheme& GetTheme() const;
    void SetTheme(std::shared_ptr<ZepTheme> spTheme);

    void SetSelection(const GlyphRange& sel);
    GlyphRange GetSelection() const;
    bool HasSelection() const;
    void ClearSelection();

    void AddRangeMarker(std::shared_ptr<RangeMarker> spMarker);
    void ClearRangeMarkers(const std::set<std::shared_ptr<RangeMarker>>& markers);
    void ClearRangeMarkers(uint32_t types);
    tRangeMarkers GetRangeMarkers(uint32_t types) const;
    void HideMarkers(uint32_t markerType);
    void ShowMarkers(uint32_t markerType, uint32_t displayType);

    void ForEachMarker(uint32_t types, SearchDirection dir, GlyphIterator begin, GlyphIterator end, std::function<bool(const std::shared_ptr<RangeMarker>&)> fnCB) const;
    std::shared_ptr<RangeMarker> FindNextMarker(GlyphIterator start, SearchDirection dir, uint32_t markerType);

    void SetBufferType(BufferType type);
    BufferType GetBufferType() const;

    void SetLastEditLocation(GlyphIterator loc);
    GlyphIterator GetLastEditLocation();

    ZepMode* GetMode() const;
    void SetMode(std::shared_ptr<ZepMode> spMode);

    using tLineWidgets = std::vector<std::shared_ptr<ILineWidget>>;
    void AddLineWidget(long line, std::shared_ptr<ILineWidget> spWidget);
    void ClearLineWidgets(long line);
    const tLineWidgets* GetLineWidgets(long line) const;

    uint64_t GetLastUpdateTime() const
    {
        return m_lastUpdateTime;
    }

    uint64_t GetUpdateCount() const
    {
        return m_updateCount;
    }

    bool IsHidden() const;

    bool HasFileFlags(uint32_t flags) const;
    void SetFileFlags(uint32_t flags, bool set = true);
    void ClearFileFlags(uint32_t flags);
    void ToggleFileFlag(uint32_t flags);

    NVec2i GetExpression(ExpressionType type, const GlyphIterator location, const std::vector<char>& beginExpression, const std::vector<char>& endExpression) const;

private:
    void ClearRangeMarker(std::shared_ptr<RangeMarker> spMarker);

    void MarkUpdate();

    void UpdateForInsert(const GlyphIterator& startOffset, const GlyphIterator& endOffset);
    void UpdateForDelete(const GlyphIterator& startOffset, const GlyphIterator& endOffset);

private:
    // Buffer & record of the line end locations
    GapBuffer<uint8_t> m_gapBuffer;
    std::vector<ByteIndex> m_lineEnds;

    // File and modification info
    ZepPath m_filePath;
    std::string m_strName;
    uint32_t m_fileFlags = 0;
    BufferType m_bufferType = BufferType::Normal;
    GlyphIterator m_lastEditLocation;// = 0;
    uint64_t m_updateCount = 0;
    uint64_t m_lastUpdateTime = 0;

    // Syntax and theme
    std::shared_ptr<ZepSyntax> m_spSyntax;
    std::shared_ptr<ZepTheme> m_spOverrideTheme;
    SyntaxProvider m_syntaxProvider;

    // Widgets
    std::map<ByteIndex, std::vector<std::shared_ptr<ILineWidget>>> m_lineWidgets;

    // Selections
    GlyphRange m_selection;
    tRangeMarkers m_rangeMarkers;

    // Modes
    std::shared_ptr<ZepMode> m_spMode;
};

// Notification payload
enum class BufferMessageType
{
    // Inform clients that we are about to mess with the buffer
    PreBufferChange = 0,
    TextChanged,
    TextDeleted,
    TextAdded,
    Loaded,
    MarkersChanged
};

struct BufferMessage : public ZepMessage
{
    BufferMessage(ZepBuffer* pBuff, BufferMessageType messageType, const GlyphIterator& startLoc, const GlyphIterator& endLoc)
        : ZepMessage(Msg::Buffer)
        , pBuffer(pBuff)
        , type(messageType)
        , startLocation(startLoc)
        , endLocation(endLoc)
    {
    }

    ZepBuffer* pBuffer;
    BufferMessageType type;
    GlyphIterator startLocation;
    GlyphIterator endLocation;
};


} // namespace Zep
