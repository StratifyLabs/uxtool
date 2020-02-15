#include "ThemeManager.hpp"
#include <sapi/var/Json.hpp>

ThemeColors::ThemeColors(
		const var::String & name,
		const var::JsonObject & object,
		const theme_color_t & background_color
		){

	set_background(background_color);

	m_is_outline = false;
	if( name.find("outline") != String::npos ){
		m_is_outline = true;
		set_border(
					import_hex_code(
						object.at("color").to_string()
						)
					);
		set_color(
					background_color
					);
		set_text(
					import_hex_code(
						object.at("color").to_string()
						)
					);
	} else {

		set_border(
					import_hex_code(object.at("border").to_string())
					);
		set_color(
					import_hex_code(object.at("color").to_string())
					);
		set_text(
					import_hex_code(object.at("text").to_string())
					);
	}
}

ThemeColors::theme_color_t ThemeColors::import_hex_code(
		const var::String & hex
		){
	theme_color_t result = {0};

	if( hex.length() != 7 ){
		return result;
	}
	result.red = hex.create_sub_string(String::Position(1), String::Length(2)).to_unsigned_long(String::base_16);
	result.green = hex.create_sub_string(String::Position(3), String::Length(2)).to_unsigned_long(String::base_16);
	result.blue = hex.create_sub_string(String::Position(5), String::Length(2)).to_unsigned_long(String::base_16);

	return result;
}

ThemeColors ThemeColors::highlight() const {
	ThemeColors result(*this);
	result.m_background = m_background;
	result.m_color = calculate_highlighted(m_color);
	return result;
}

ThemeColors ThemeColors::disable() const {
	ThemeColors result(*this);

	return result;
}

var::Vector<ThemeColors::theme_color_t>
ThemeColors::create_palette_colors(
		u8 bits_per_pixel
		) const {

	var::Vector<theme_color_t> result( 1<<bits_per_pixel );

	if( bits_per_pixel == 1 ){
		result.at(0) = m_color;
		result.at(1) = m_text;
	} else if( bits_per_pixel == 2 ){
		result.at(0) = m_background;
		result.at(1) = m_color;
		result.at(2) = m_text;
		result.at(3) = m_border;
	} else {
		result.at(0) = m_background;
		size_t blend_count = result.count()-2;
		for(size_t i=0; i < blend_count; i++){
			theme_color_t color;
			u32 tmp = (m_color.red * (blend_count - i) + m_text.red * i + blend_count) / (2*blend_count);
			color.red = tmp;
			tmp = (m_color.green * (blend_count - i) + m_text.green * i + blend_count) / (2*blend_count);
			color.green = tmp;
			tmp = (m_color.blue * (blend_count - i) + m_text.blue * i + blend_count) / (2*blend_count);
			color.blue = tmp;
		}

		result.at(blend_count + 1) = m_border;
	}
	return result;
}

ThemeColors::theme_color_t ThemeColors::calculate_highlighted(
		const theme_color_t & color
		) const{
	theme_color_t result;
	result.red = color.red / 4;
	result.green = color.green / 4;
	result.blue = color.blue / 4;
	return result;
}

ThemeColors::theme_color_t ThemeColors::calculate_disabled(const theme_color_t & color) const{
	theme_color_t result;
	result.red = 255 - color.red / 4;
	result.green = 255 - color.green / 4;
	result.blue = 255 - color.blue / 4;
	return result;
}

ThemeManager::ThemeManager() : m_theme(m_theme_file)
{

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
	switch(value){
		case Theme::style_dark: return "dark";
		case Theme::style_light: return "light";
		case Theme::style_brand_primary: return "brandPrimary";
		case Theme::style_brand_secondary: return "brandSecondary";
		case Theme::style_info: return "info";
		case Theme::style_success: return "success";
		case Theme::style_warning: return "warning";
		case Theme::style_danger: return "danger";
		case Theme::style_outline_dark: return "outlineDark";
		case Theme::style_outline_light: return "outlineLight";
		case Theme::style_outline_brand_primary: return "outlineBrandPrimary";
		case Theme::style_outline_brand_secondary: return "outlineBrandSecondary";
		case Theme::style_outline_info: return "outlineInfo";
		case Theme::style_outline_success: return "outlineSuccess";
		case Theme::style_outline_warning: return "outlineWarning";
		case Theme::style_outline_danger: return "outlineDanger";
	}
	return "";
}

var::String ThemeManager::get_state_name(enum Theme::state value){
	switch(value){
		case Theme::state_default: return "default";
		case Theme::state_disabled: return "disabled";
		case Theme::state_highlighted: return "highlighted";
	}
	return "";
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
				Theme::PixelFormat(0)
				) < 0 ){
		printer().error("failed to save theme to " + output_path);
		return -1;
	}

	theme_color_t background_color =
			ThemeColors::import_hex_code(
				input.at("background").to_string()
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

		set_color(
					get_theme_style(style),
					Theme::state_default,
					theme_colors,
					bits_per_pixel
					);

		set_color(
					get_theme_style(style),
					Theme::state_highlighted,
					theme_colors.highlight(),
					bits_per_pixel
					);

		set_color(
					get_theme_style(style),
					Theme::state_disabled,
					theme_colors.disable(),
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
		u8 bits_per_pixel){

	var::Vector<ThemeColors::theme_color_t> colors =
			base_colors.create_palette_colors(bits_per_pixel);
	var::Vector<u16> colors_rgb;

	//output format RGB565??
	for(const auto & color: colors){
		colors_rgb.push_back(ThemeColors::rgb565(color));
	}
	m_theme_file.write(colors_rgb);

}

u16 ThemeManager::mix(
		const theme_color_t & first,
		const theme_color_t & second){
	//first has 66% weigth, second has 33% weight
	u32 red = (first.red * 666 + second.red * 333 + 500)/1000;
	u32 green = (first.green * 666 + second.green * 333 + 500)/1000;
	u32 blue = (first.blue * 666 + second.blue * 333 + 500)/1000;
	u16 rgb = (red & 0xf8) << 8;
	rgb |= (green & 0xfc) << 3;
	rgb |= (blue & 0xf8) >> 3;
	printer().debug(
				String().format("mix %02X %02X %02X + %02X %02X %02X = %02X %02X %02X = 0x%04X",
												first.red, first.green, first.blue,
												second.red, second.green, second.blue,
												red, green, blue,
												rgb
												));
	return rgb;
}

