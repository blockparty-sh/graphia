#include "string.h"

#include <QStringList>

#include <cmath>

bool u::isNumeric(const std::string& string)
{
    std::size_t pos;
    long double value = 0.0;

    try
    {
        value = std::stold(string, &pos);
    }
    catch(std::invalid_argument&)
    {
        return false;
    }
    catch(std::out_of_range&)
    {
        return false;
    }

    return pos == string.size() && !std::isnan(value);
}

std::vector<QString> u::toQStringVector(const QStringList& stringList)
{
    std::vector<QString> v;

    for(const auto& string : stringList)
        v.emplace_back(string);

    return v;
}

// https://stackoverflow.com/a/6089413/2721809
std::istream& u::getline(std::istream& is, std::string& t)
{
    t.clear();

    // The characters in the stream are read one-by-one using a std::streambuf.
    // That is faster than reading them one-by-one using the std::istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    std::istream::sentry se(is, true);
    Q_UNUSED(se);
    std::streambuf* sb = is.rdbuf();

    while(true)
    {
        int c = sb->sbumpc();
        switch(c)
        {
        case '\n':
            return is;

        case '\r':
            if(sb->sgetc() == '\n')
                sb->sbumpc();
            return is;

        case EOF:
            // Also handle the case when the last line has no line ending
            if(t.empty())
                is.setstate(std::ios::eofbit);
            return is;

        default:
            t += static_cast<char>(c);
        }
    }
}