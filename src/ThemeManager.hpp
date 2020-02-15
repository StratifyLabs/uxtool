#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <sapi/fs.hpp>
#include <sapi/var.hpp>
#include <sapi/sgfx.hpp>

#include "ApplicationPrinter.hpp"

class ThemeFlags {
public:
	typedef struct {
		u8 red;
		u8 green;
		u8 blue;
	} theme_color_t;

};

class ThemeColors : public ThemeFlags {
public:

	ThemeColors(
			const var::String & name,
			const var::JsonObject & object,
			const theme_color_t & background_color
			);

	ThemeColors& set_background(const theme_color_t & value){
		m_background = value;
		return *this;
	}

	ThemeColors& set_color(const theme_color_t & value){
		m_color = value;
		return *this;
	}

	ThemeColors& set_text(const theme_color_t & value){
		m_text = value;
		return *this;
	}

	ThemeColors& set_border(const theme_color_t & value){
		m_border = value;
		return *this;
	}

	ThemeColors highlight() const;
	ThemeColors disable() const;
	var::Vector<theme_color_t> create_palette_colors(u8 bits_per_pixel) const;

	static theme_color_t import_hex_code(const var::String & hex);

	static u16 rgb565(const theme_color_t & color){
		u16 rgb = (color.red & 0xf8) << 8;
		rgb |= (color.green & 0xfc) << 3;
		rgb |= (color.blue & 0xf8) >> 3;
		return rgb;
	}

private:
	theme_color_t m_background;
	theme_color_t m_color;
	theme_color_t m_text;
	theme_color_t m_border;
	bool m_is_outline;


	theme_color_t calculate_highlighted(const theme_color_t & color) const;
	theme_color_t calculate_disabled(const theme_color_t & color) const;

};

class ThemeManager : public ApplicationPrinter, public ThemeFlags {
public:
	ThemeManager();

	/*! \details Imports a theme from a JSON file.
		*
		* ```json
		* {
		*   "light": {
		*     "background": "#ffffff",
		*     "border": "#cccccc",
		*     "color": "#ffffff",
		*     "text": "#222222",
		*   },
		*   "dark": {
		*     "background": "#ffffff",
		*     "border": "#000000",
		*     "color": "#222222",
		*     "text": "#ffffff",
		*   },
		* }
		* ```
		*
		* Also add, primary, secondary, info, success, warning, danger
		*
		*
		*/
	int import(fs::File::SourcePath input,
						 fs::File::DestinationPath output);

private:

	//holds the RGB values for all styles and states
	sgfx::Theme m_theme;
	File m_theme_file;

	u16 mix(const theme_color_t & first, const theme_color_t & second);
	theme_color_t calculate_highlighted(const theme_color_t & color);
	theme_color_t calculate_disabled(const theme_color_t & color);

	void set_color(enum Theme::style style,
								 enum Theme::state state,
								 const ThemeColors& base_colors,
								 u8 bits_per_pixel
								 );

	enum sgfx::Theme::style get_theme_style(const var::String & style_name);
	var::String get_style_name(enum Theme::style value);
	var::String get_state_name(enum Theme::state value);
	var::Vector<var::String> get_styles() const;
	var::Vector<var::String> get_states() const;
	theme_color_t import_hex_code(const String & hex) const;

	int import_object(
			const var::JsonObject& input,
			const var::String& output,
			const String& pixel_format,
			u8 bits_per_pixel
			);
};

#endif // THEMEMANAGER_HPP
