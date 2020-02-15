#ifndef APPLICATIONPRINTER_HPP
#define APPLICATIONPRINTER_HPP

#include <sapi/sys.hpp>

class ApplicationPrinter {
public:
	static YamlPrinter & printer(){ return m_printer; }

private:
	static YamlPrinter m_printer;
};

typedef ApplicationPrinter Ap;

#endif // APPLICATIONPRINTER_HPP
