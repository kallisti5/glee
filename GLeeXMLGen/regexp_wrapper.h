#pragma once

#include <regexp/Matcher.h>
#include <regexp/Pattern.h>
#include <vector>

class regex
{
public:
    explicit regex(const char* pattern)
    {
        m_pattern = Pattern::compile( pattern );
		if( m_pattern != NULL )
			m_matcher = m_pattern->createMatcher("");
		else
			m_matcher = NULL;
    }
    ~regex()
    {
        delete m_matcher;
        delete m_pattern;
    }
    
    Pattern* m_pattern;
    Matcher* m_matcher;
};

struct match
{
    const char* first;
    const char* second;
};

class cmatch
{
public:
    void Init( const char* text, const regex& reg )
    {
        m_matches.resize( reg.m_matcher->getNumGroups()+1 );
        for( int i = 0; i < reg.m_matcher->getNumGroups()+1; ++i )
        {
            m_matches[i].first  = text + reg.m_matcher->getStartingIndex(i);
            m_matches[i].second = text + reg.m_matcher->getEndingIndex(i);
        }
    }
    
    const match& operator [] ( int i )
    {
        return m_matches[i];
    }
private:
    std::vector<match> m_matches;
};

bool regex_match( const char* text, cmatch& groups, regex& reg )
{
	if( reg.m_pattern == NULL )
		return false;
    reg.m_matcher->setString( text );
    bool ret = reg.m_matcher->matches();
    groups.Init( text, reg );
    return ret;
}

bool regex_search( const char* text, cmatch& groups, regex& reg )
{
	if( reg.m_pattern == NULL )
		return false;
    reg.m_matcher->setString( text );
    bool ret = reg.m_matcher->findNextMatch();
    groups.Init( text, reg );
    return ret;
}