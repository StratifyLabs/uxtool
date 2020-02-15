#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <sapi/fs.hpp>
#include <sapi/var.hpp>
#include <sapi/sgfx.hpp>

#include "ApplicationPrinter.hpp"

class ThemeFlags : public ApplicationPrinter {
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
			const PaletteColor & background_color
			);

	ThemeColors& set_background(const PaletteColor & value){
		m_background = value;
		return *this;
	}

	ThemeColors& set_color(const PaletteColor & value){
		m_color = value;
		return *this;
	}

	ThemeColors& set_text(const PaletteColor & value){
		m_text = value;
		return *this;
	}

	ThemeColors& set_border(const PaletteColor & value){
		m_border = value;
		return *this;
	}

	const PaletteColor & background() const {
		return m_background;
	}

	const PaletteColor & border() const {
		return m_border;
	}

	const PaletteColor & color() const {
		return m_color;
	}

	const PaletteColor & text() const {
		return m_text;
	}

	ThemeColors highlight() const;
	ThemeColors disable() const;
	Palette create_palette(
			const String& pixel_format,
			u8 bits_per_pixel
			) const;

private:
	PaletteColor m_background;
	PaletteColor m_color;
	PaletteColor m_text;
	PaletteColor m_border;
	bool m_is_outline;


	PaletteColor calculate_highlighted(const PaletteColor & color) const;
	PaletteColor calculate_disabled(const PaletteColor& color) const;

};

class ThemeManager : public ThemeFlags {
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

	void set_color(
			enum Theme::style style,
			enum Theme::state state,
			const ThemeColors& base_colors,
			const String& pixel_format,
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
