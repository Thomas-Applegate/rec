#include "dfa.h"

Regex_Exception::Regex_Exception(const char* what) noexcept : m_what(what) {}
Regex_Exception::Regex_Exception(const Regex_Exception& oth) noexcept : m_what(oth.m_what) {}
Regex_Exception& Regex_Exception::operator=(const Regex_Exception& oth) noexcept
{
	m_what = oth.m_what;
	return *this;
}
const char* Regex_Exception::what() const noexcept { return m_what; }

/* regex cfg
S  -> G S' $
S' -> pipe S | eps
G  -> U O G | eps
U  -> ch | . | ( S ) | A
O -> * | + | ? | { I N } | eps
A  -> [ ch - ch A' ]
A' -> ch - ch A' | eps
I -> digit I'
I' -> digit I' | eps
N -> - I | + | eps
*/

Nfa::Nfa(const std::string& regex) {}

Nfa::operator const std::vector<Nfa::state>&() const noexcept { return m_states; }
const std::vector<Nfa::state>& Nfa::states() const noexcept { return m_states; }

Dfa::Dfa(const Nfa& nfa) {}

Dfa::operator const std::vector<Dfa::state>&() const noexcept { return m_states; }
const std::vector<Dfa::state>& Dfa::states() const noexcept { return m_states; }
