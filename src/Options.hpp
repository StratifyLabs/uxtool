#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <sapi/sys.hpp>
#include <sapi/var.hpp>

#include "Object.hpp"

class Options : public Object{
public:
	Options(const Cli & cli);

	bool is_valid() const {
		return m_is_valid;
	}

	void show();

	const String& bits_per_pixel() const {
		return m_bits_per_pixel;
	}
	const String& action() const {
		return m_action;
	}
	const String& input() const {
		return m_input;
	}
	const String& output() const {
		return m_output;
	}

	String output_file_path() const {
		String result;
		if( File::get_info(output()).is_directory() ){
			result = output() + "/" + FileInfo::base_name(input());
			return result;
		}
		if( output().is_empty() ){
			return FileInfo::no_suffix(input());
		}
		return output();
	}

	const String& point_size() const {
		return m_point_size;
	}
	const String& pour_size() const {
		return m_pour_size;
	}
	const String& downsample_size() const {
		return m_downsample_size;
	}
	const String& input_suffix() const {
		return m_input_suffix;
	}
	const String& output_suffix() const {
		return m_output_suffix;
	}
	bool is_theme() const {
		return m_is_theme;
	}
	bool is_icon() const {
		return m_is_icon;
	}
	bool is_overwrite() const {
		return m_is_overwrite;
	}
	bool is_map() const {
		return m_is_map;
	}
	bool is_json() const {
		return m_is_json;
	}
	bool is_details() const {
		return m_is_details;
	}
	bool is_json_printer() const {
		return m_is_json_printer;
	}

	bool is_help() const {
		return m_help == "true";
	}

	const String& characters() const {
		return m_characters;
	}


private:

	bool m_is_valid = true;
	String m_bits_per_pixel;
	String m_action;
	String m_input;
	String m_output;
	String m_point_size;
	String m_pour_size;
	String m_downsample_size;
	String m_help;
	String m_characters;
	String m_input_suffix;
	String m_output_suffix;
	bool m_is_theme;
	bool m_is_icon;
	bool m_is_overwrite;
	bool m_is_map;
	bool m_is_json;
	bool m_is_details;
	bool m_is_json_printer;

	StringList m_hint_list;




};

#endif // OPTIONS_HPP
