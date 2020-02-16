#include "ThemeManager.hpp"
#include <sapi/var/Json.hpp>

ThemeColors::ThemeColors(
		const var::String & name,
		const var::JsonObject & object,
		const PaletteColor & background_color
		){

	set_background(background_color);

	m_is_outline = false;
	if( name.find("outline") != String::npos ){
		m_is_outline = true;
		set_border(
					PaletteColor(object.at("color").to_string())
					);
		set_color(
					background_color
					);
		set_text(
					PaletteColor(object.at("color").to_string())
					);
	} else {

		set_border(
					PaletteColor(object.at("border").to_string())
					);
		set_color(
					PaletteColor(object.at("color").to_string())
					);
		set_text(
					PaletteColor(object.at("text").to_string())
					);
	}
}

ThemeColors ThemeColors::highlight() const {
	ThemeColors result(*this);
	result.m_background = m_background;
	result.m_color = calculate_highlighted(m_color);
	return result;
}

ThemeColors ThemeColors::disable() const {
	ThemeColors result(*this);
	result.m_background = m_background;
	result.m_color = calculate_disabled(m_color);
	return result;
}

Palette ThemeColors::create_palette(
		const String & pixel_format,
		u8 bits_per_pixel
		) const {

	Palette result;

	result.set_color_count(
				Palette::get_color_count(bits_per_pixel)
				);

	result.set_pixel_format(
				Palette::decode_pixel_format(pixel_format)
				);

	if( result.is_valid() == false ){
		printer().error(
					"failed to create palette with pixel format: " +
					pixel_format +
					" and bits per pixel:" +
					String::number(bits_per_pixel)
					);
		return Palette();
	}

	if( bits_per_pixel == 1 ){
		result.assign_color(0, m_color);
		result.assign_color(1, m_text);
	} else if( bits_per_pixel == 2 ){
		result.assign_color(0, m_background);
		result.assign_color(1, m_color);
		result.assign_color(2, m_text);
		result.assign_color(3, m_border);
	} else {
		result.assign_color(0, m_background);
		enum Palette::color_count color_count = Palette::get_color_count( bits_per_pixel );

		const size_t blend_count = color_count - 2;
		var::Vector<PaletteColor> gradient =
				PaletteColor::calculate_gradient(
					m_color,
					m_text,
					blend_count
					);

		for(size_t i=0; i < gradient.count(); i++){
			printer().debug("gradient color is " + gradient.at(i).to_hex_code());
			result.assign_color(i+1, gradient.at(i));
			printer().debug(
						"gradient result is " +
						PaletteColor(result.colors().at(i+1)).to_hex_code()
						);
		}
		result.assign_color(color_count - 1, m_border);
	}
	return result;
}

PaletteColor ThemeColors::calculate_highlighted(
		const PaletteColor& color
		) const{
	return color * 0.75f;
}

PaletteColor ThemeColors::calculate_disabled(
		const PaletteColor & color
		) const{
	return color * 1.75f;
}

ThemeManager::ThemeManager() : m_theme(m_theme_file){

}

var::Vector<var::String> ThemeManager::get_styles() const {
	var::Vector<var::String> result;
	//these must be in the correct order
	result.push_back("dark");
	result.push_back("light");
	result.push_back("brandPrimary");
	result.push_back("brandSecondary");
	result.push_back("info");
	result.push_back("success");
	result.push_back("warning");
	result.push_back("danger");
	result.push_back("outlineDark");
	result.push_back("outlineLight");
	result.push_back("outlineBrandPrimary");
	result.push_back("outlineBrandSecondary");
	result.push_back("outlineInfo");
	result.push_back("outlineSuccess");
	result.push_back("outlineWarning");
	result.push_back("outlineDanger");
	return result;
}

var::String ThemeManager::get_style_name(enum Theme::style value){
	return Theme::get_style_name(value);
}

var::String ThemeManager::get_state_name(enum Theme::state value){
	return Theme::get_state_name(value);
}

enum sgfx::Theme::style ThemeManager::get_theme_style(const var::String & style_name){
	if( style_name == "dark" ){ return sgfx::Theme::style_dark; }
	if( style_name == "light" ){ return sgfx::Theme::style_light; }
	if( style_name == "brandPrimary" ){ return sgfx::Theme::style_brand_primary; }
	if( style_name == "brandSecondary" ){ return sgfx::Theme::style_brand_secondary; }
	if( style_name == "info" ){ return sgfx::Theme::style_info; }
	if( style_name == "success" ){ return sgfx::Theme::style_success; }
	if( style_name == "warning" ){ return sgfx::Theme::style_warning; }
	if( style_name == "danger" ){ return sgfx::Theme::style_danger; }

	if( style_name == "outlineDark" ){ return sgfx::Theme::style_outline_dark; }
	if( style_name == "outlineLight" ){ return sgfx::Theme::style_outline_light; }
	if( style_name == "outlineBrandPrimary" ){ return sgfx::Theme::style_outline_brand_primary; }
	if( style_name == "outlineBrandSecondary" ){ return sgfx::Theme::style_outline_brand_secondary; }
	if( style_name == "outlineInfo" ){ return sgfx::Theme::style_outline_info; }
	if( style_name == "outlineSuccess" ){ return sgfx::Theme::style_outline_success; }
	if( style_name == "outlineWarning" ){ return sgfx::Theme::style_outline_warning; }
	if( style_name == "outlineDanger" ){ return sgfx::Theme::style_outline_danger; }

	//error
	printer().error("unrecognized style name " + style_name);
	return sgfx::Theme::style_light;
}

ThemeManager::theme_color_t ThemeManager::import_hex_code(const var::String & hex) const {
	theme_color_t result = {0};

	if( hex.length() != 7 ){
		printer().error("invalid hex code format " + hex + " use `#aaddcc`");
		return result;
	}
	result.red = hex.create_sub_string(String::Position(1), String::Length(2)).to_unsigned_long(String::base_16);
	result.green = hex.create_sub_string(String::Position(3), String::Length(2)).to_unsigned_long(String::base_16);
	result.blue = hex.create_sub_string(String::Position(5), String::Length(2)).to_unsigned_long(String::base_16);

	return result;
}

var::Vector<var::String> ThemeManager::get_states() const {
	var::Vector<var::String> result;

	result.push_back("default");
	result.push_back("highlighted");
	result.push_back("disabled");
	return result;
}



int ThemeManager::import(
		File::SourcePath input,
		File::DestinationPath output
		){
	var::JsonObject configuration = var::JsonDocument().load(
				fs::File::Path(input.argument())
				).to_object();

	if( configuration.is_valid() == false ){
		printer().error("failed to load path " + input.argument());
		return -1;
	}


	u8 bits_per_pixel = configuration.at("bitsPerPixel").to_integer();
	if( bits_per_pixel == 0 ){
		bits_per_pixel = 4;
	}

	String pixel_format = configuration.at("pixelFormat").to_string();

	JsonArray theme_array;
	theme_array = configuration.at("themes").to_array();

	for(u32 i=0; i < theme_array.count(); i++){
		if( import_object(
					theme_array.at(i).to_object(),
					output.argument(),
					pixel_format,
					bits_per_pixel
					) < 0 ){
			return -1;
		}
	}

	return 0;
}

int ThemeManager::import_object(
		const var::JsonObject & input,
		const var::String & output,
		const var::String & pixel_format,
		u8 bits_per_pixel
		){

	String output_path;
	String name = input.at("name").to_string();

	if( name.is_empty() ){
		printer().error("name of theme not provided");
		return -1;
	}

	{
		String file_name = name + ".sth";
		output_path = output;
		if( output.is_empty() == false ){
			output_path << "/";
		}
		output_path << file_name;
	}

	if( m_theme.create(
				output_path,
				fs::File::IsOverwrite(true),
				Theme::BitsPerPixel(bits_per_pixel),
				Palette::decode_pixel_format(pixel_format)
				) < 0 ){
		printer().error("failed to save theme to " + output_path);
		return -1;
	}

	PaletteColor background_color(
				input.at("background").to_string()
				);

	printer().debug(
				"background color is " +
				background_color.to_hex_code()
				);

	for(const auto & style: get_styles() ){
		var::JsonObject style_object =
				input.at(style).to_object();

		printer().message("processing " + style);
		if( style_object.is_valid() == false ){
			printer().error("input missing style " + style);
			return -1;
		}
		ThemeColors theme_colors(
					style,
					style_object,
					background_color
					);

		printer().message("background is " + theme_colors.background().to_hex_code());
		printer().message("color is " + theme_colors.color().to_hex_code());
		printer().message("text is " + theme_colors.text().to_hex_code());
		printer().message("border is " + theme_colors.border().to_hex_code());

		set_color(
					get_theme_style(style),
					Theme::state_default,
					theme_colors,
					pixel_format,
					bits_per_pixel
					);

		set_color(
					get_theme_style(style),
					Theme::state_highlighted,
					theme_colors.highlight(),
					pixel_format,
					bits_per_pixel
					);

		set_color(
					get_theme_style(style),
					Theme::state_disabled,
					theme_colors.disable(),
					pixel_format,
					bits_per_pixel
					);

	}

	m_theme_file.close();

	printer().info("saved theme to " + output_path);

	return 0;
}

void ThemeManager::set_color(
		enum Theme::style style,
		enum Theme::state state,
		const ThemeColors & base_colors,
		const String & pixel_format,
		u8 bits_per_pixel){

	Palette palette = base_colors.create_palette(
				pixel_format,
				bits_per_pixel
				);

	printer().open_array(
				"palette " +
				get_style_name(style) +
				" " +
				get_state_name(state),
				Printer::level_message
				);
	for(const auto & color: palette.colors()){
		printer() << PaletteColor(color).to_hex_code();
	}
	printer().close_array();


	m_theme.write_palette(
				style,
				state,
				palette.colors()
				);

}


