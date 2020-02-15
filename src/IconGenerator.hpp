#ifndef ICONGENERATOR_HPP
#define ICONGENERATOR_HPP

#include "FontObject.hpp"

class IconGeneratorIcon {
public:

	IconGeneratorIcon(
			const Bitmap & bitmap,
			const sg_font_icon_t & icon){
		m_bitmap = bitmap;
		m_icon = icon;
		m_icon.width = bitmap.width();
		m_icon.height = bitmap.height();
	}

	String name() const {
		return String(m_icon.name);
	}

	bool operator == (const IconGeneratorIcon & a) const {
		return String(icon().name) == String(a.icon().name);
	}

	bool operator < (const IconGeneratorIcon & a) const {
		return String(icon().name) < String(a.icon().name);
	}

	bool operator > (const IconGeneratorIcon & a) const {
		return String(icon().name) > String(a.icon().name);
	}

	const Bitmap & bitmap() const {
		return m_bitmap;
	}

	const sg_font_icon_t & icon() const {
		return m_icon;
	}

	sg_font_icon_t & icon(){
		return m_icon;
	}

private:
	Bitmap m_bitmap;
	sg_font_icon_t m_icon;
};


class IconGenerator : public FontObject {
public:

	int generate_icon_file(const String & destination);

	void add_icon(const IconGeneratorIcon & icon){
		m_icon_list.push_back(icon);
	}

	void clear(){
		m_icon_list.clear();
	}

private:
	var::Vector<IconGeneratorIcon> m_icon_list;

	var::Vector<Bitmap> build_master_canvas(const sg_font_icon_header_t & header);
	const var::Vector<IconGeneratorIcon> & icon_list() const {
		return m_icon_list;
	}

	var::Vector<IconGeneratorIcon> & icon_list(){
		return m_icon_list;
	}
};

#endif // ICONGENERATOR_HPP
