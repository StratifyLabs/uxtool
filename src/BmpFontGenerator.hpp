#ifndef BMPFONTGENERATOR_HPP
#define BMPFONTGENERATOR_HPP

#include "Object.hpp"

class BmpFontGeneratorCharacter {
public:

	BmpFontGeneratorCharacter(
			const Bitmap & bitmap,
			const sg_font_char_t & font_character){
		m_bitmap = bitmap;
		m_font_character = font_character;
	}

	bool operator == (const BmpFontGeneratorCharacter & a) const {
		return font_character().id == a.font_character().id;
	}

	bool operator < (const BmpFontGeneratorCharacter & a) const {
		return font_character().id < a.font_character().id;
	}

	bool operator > (const BmpFontGeneratorCharacter & a) const {
		return font_character().id > a.font_character().id;
	}

	const Bitmap & bitmap() const {
		return m_bitmap;
	}

	const sg_font_char_t & font_character() const {
		return m_font_character;
	}

	sg_font_char_t & font_character(){
		return m_font_character;
	}

private:
	Bitmap m_bitmap;
	sg_font_char_t m_font_character;
};

class BmpFontGenerator : public Object {
public:
	BmpFontGenerator();

	void set_map_output_file(const String & path){
		m_map_output_file = path;
	}

	void clear(){
		m_character_list.clear();
	}

	int import_map(const String & map);
	int generate_font_file(const String & destination);

	var::Vector<BmpFontGeneratorCharacter> & character_list(){ return m_character_list; }
	const var::Vector<BmpFontGeneratorCharacter> & character_list() const { return m_character_list; }

	var::Vector<sg_font_kerning_pair_t> & kerning_pair_list(){ return m_kerning_pair_list; }
	const var::Vector<sg_font_kerning_pair_t> & kerning_pair_list() const { return m_kerning_pair_list; }

	//var::Vector<Bitmap> & bitmap_list(){ return m_bitmap_list; }
	//const var::Vector<Bitmap> & bitmap_list() const { return m_bitmap_list; }

	void set_is_ascii(bool value = true){ m_is_ascii = true; }

private:
	int generate_map_file(const sg_font_header_t & header, const var::Vector<Bitmap> & master_canvas_list);

	var::String m_map_output_file;
	bool m_is_ascii;
	var::Vector<Bitmap> build_master_canvas(const sg_font_header_t & header);
	//var::Vector<sg_font_char_t> m_character_list;
	//var::Vector<Bitmap> m_bitmap_list;
	var::Vector<sg_font_kerning_pair_t> m_kerning_pair_list;
	var::Vector<BmpFontGeneratorCharacter> m_character_list;

};

#endif // BMPFONTGENERATOR_HPP
