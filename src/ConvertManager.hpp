#ifndef CONVERTMANAGER_HPP
#define CONVERTMANAGER_HPP

#include "Object.hpp"
#include "BmpFontGenerator.hpp"
#include "IconGenerator.hpp"
#include "Options.hpp"

class ConvertManager : public Object {
public:
	ConvertManager();

	bool convert(const Options & options);

	int process_font(
			File::SourcePath source_file_path,
			File::DestinationPath destination_directory_path
			);

	int process_icons(
			File::SourcePath source_file_path,
			File::DestinationPath destination_file_path
			);

	void set_point_size(sg_size_t value){
		m_point_size = value;
	}

	sg_size_t point_size() const {
		return m_point_size;
	}


private:
	sg_size_t m_point_size;
	BmpFontGenerator m_bmp_font_generator; //used for exporting to bmp
	IconGenerator m_icon_generator;
	String m_font_path;
	String m_density;

	static const String temporary_directory(){
		return "fonttoolTemporary";
	}

	static const String temporary_unicode_text(){
		return temporary_directory() + "/unicode.txt";
	}

	static String unicode_bitmap_path(u16 unicode){
		return temporary_directory() + "/" + String().format("0x%04x", unicode) + ".bmp";
	}

	static String temporary_icon_path(const var::String & name){
		return temporary_directory() + "/" + FileInfo::base_name(name) + ".bmp";
	}

	int process_icon(const var::String & path);
	int generate_icon_bitmap(const var::String & name);
	int generate_icon_details(const var::String & name);

	int process_character(u16 unicode);
	int generate_character_bitmap(u16 unicode);
	int generate_character_details(u16 unicode);

	void sanitize_bitmap(Bitmap & bitmap);

};

#endif // CONVERTMANAGER_HPP
