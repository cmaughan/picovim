#include "zep/glyph_iterator.h"
#include "zep/buffer.h"

namespace Zep
{

GlyphIterator::GlyphIterator(const ZepBuffer* buffer, unsigned long offset)
    : m_pBuffer(buffer)
{
    if (buffer)
    {
        m_index = offset;
    }
}

GlyphIterator::GlyphIterator(const GlyphIterator& itr)
    : m_pBuffer(itr.m_pBuffer)
    , m_index(itr.m_index)
{
}

long GlyphIterator::Index() const
{
    return m_index;
}

bool GlyphIterator::Valid() const
{
    if (!m_pBuffer)
    {
        return false;
    }

    if (m_index < 0 || m_index >= m_pBuffer->GetGapBuffer().size())
    {
        return false;
    }
    return true;
}

bool GlyphIterator::operator<(const GlyphIterator& rhs) const
{
    return m_index < rhs.m_index;
}

bool GlyphIterator::operator<=(const GlyphIterator& rhs) const
{
    return m_index <= rhs.m_index;
}
bool GlyphIterator::operator>(const GlyphIterator& rhs) const
{
    return m_index > rhs.m_index;
}

bool GlyphIterator::operator>=(const GlyphIterator& rhs) const
{
    return m_index >= rhs.m_index;
}

bool GlyphIterator::operator==(const GlyphIterator& rhs) const
{
    return m_index == rhs.m_index;
}

bool GlyphIterator::operator!=(const GlyphIterator& rhs) const
{
    return m_index != rhs.m_index;
}

GlyphIterator& GlyphIterator::operator=(const GlyphIterator& rhs)
{
    m_pBuffer = rhs.m_pBuffer;
    m_index = rhs.m_index;
    return *this;
}

uint8_t GlyphIterator::Char() const
{
    if (!m_pBuffer)
    {
        return 0;
    }
    return m_pBuffer->GetGapBuffer()[m_index];
}

uint8_t GlyphIterator::operator*() const
{
    if (!m_pBuffer)
    {
        return 0;
    }
    return m_pBuffer->GetGapBuffer()[m_index];
}

GlyphIterator& GlyphIterator::MoveClamped(long count, LineLocation clamp)
{
    if (!m_pBuffer)
    {
        return *this;
    }
    auto& gapBuffer = m_pBuffer->GetGapBuffer();

    if (count >= 0)
    {
        auto lineEnd = m_pBuffer->GetLinePos(*this, clamp);
        for (long c = 0; c < count; c++)
        {
            if (m_index >= lineEnd.m_index)
            {
                break;
            }
            m_index += utf8_codepoint_length(gapBuffer[m_index]);
        }
    }
    else
    {
        auto lineBegin = m_pBuffer->GetLinePos(*this, LineLocation::LineBegin);
        for (long c = count; c < 0; c++)
        {
            while ((m_index > lineBegin.Index()) && utf8_is_trailing(gapBuffer[--m_index]));
        }
    }

    Clamp();

    return *this;
}

GlyphIterator& GlyphIterator::Move(long count)
{
    if (!m_pBuffer)
    {
        return *this;
    }
    auto& gapBuffer = m_pBuffer->GetGapBuffer();

    if (count >= 0)
    {
        for (long c = 0; c < count; c++)
        {
            m_index += utf8_codepoint_length(gapBuffer[m_index]);
        }
    }
    else
    {
        for (long c = count; c < 0; c++)
        {
            while ((m_index > 0) && utf8::internal::is_trail(gapBuffer[--m_index]));
        }
    }
    Clamp();
    return *this;
}

GlyphIterator GlyphIterator::Clamped() const
{
    GlyphIterator itr(*this);
    itr.Clamp();
    return itr;
}

GlyphIterator& GlyphIterator::Clamp()
{
    // Invalid thing is still invalid
    if (!m_pBuffer)
    {
        return *this;
    }

    // Clamp to the last char, not the 0 on the end
    m_index = std::min(m_index, long(m_pBuffer->GetGapBuffer().size()) - 1);
    m_index = std::max(m_index, 0l);
    return *this;
}

void GlyphIterator::Invalidate()
{
    m_index = -1;
    m_pBuffer = nullptr;
}

GlyphIterator GlyphIterator::Peek(long count) const
{
    GlyphIterator copy(*this);
    copy.Move(count);
    return copy;
}

GlyphIterator GlyphIterator::PeekClamped(long count, LineLocation clamp) const
{
    GlyphIterator copy(*this);
    copy.MoveClamped(count, clamp);
    return copy;
}

GlyphIterator GlyphIterator::PeekByteOffset(long count) const
{
    return GlyphIterator(m_pBuffer, m_index + count);
}

GlyphIterator GlyphIterator::operator--(int)
{
    GlyphIterator ret(*this);
    Move(-1);
    return ret;
}

GlyphIterator GlyphIterator::operator++(int)
{
    GlyphIterator ret(*this);
    Move(1);
    return ret;
}

GlyphIterator GlyphIterator::operator+(long value) const
{
    GlyphIterator ret(*this);
    ret.Move(value);
    return ret;
}

GlyphIterator GlyphIterator::operator-(long value) const
{
    GlyphIterator ret(*this);
    ret.Move(-value);
    return ret;
}

void GlyphIterator::operator+=(long count)
{
    Move(count);
}

void GlyphIterator::operator-=(long count)
{
    Move(-count);
}


GlyphRange::GlyphRange(GlyphIterator a, GlyphIterator b)
    : first(a)
    , second(b)
{
}

GlyphRange::GlyphRange()
{
}

bool GlyphRange::ContainsLocation(long loc) const
{
    return loc >= first.Index() && loc < second.Index();
}

bool GlyphRange::ContainsLocation(GlyphIterator loc) const
{
    return loc >= first && loc < second;
}


/*
long operator-(const GlyphIterator& lhs, const GlyphIterator& rhs)
{
    return lhs.Index() - rhs.Index();
}
*/



} // namespace Zep
