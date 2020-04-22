#include <sapi/var.hpp>
#include "ConvertManager.hpp"

ConvertManager::ConvertManager(){}

bool ConvertManager::convert(const Options & options){
	set_bits_per_pixel(options.bits_per_pixel().to_integer());
	set_point_size( options.point_size().to_integer() );
	set_character_set(options.characters());
	set_generate_map(options.is_map());

	if( options.input_suffix() == "ttf" || options.input_suffix() == "otf"){
		process_font(
					File::SourcePath(options.input()),
					File::DestinationPath(options.output())
					);
		return true;
	} else if( options.input_suffix() == "svg" || options.is_icon() ){
		process_icons(
					File::SourcePath(options.input()),
					File::DestinationPath(options.output())
					);
		return true;
	}
	return false;
}

int ConvertManager::process_icons(
		File::SourcePath source_file_path,
		File::DestinationPath destination_file_path
		){

	m_icon_generator.clear();
	String input_path = source_file_path.argument();
	String output_path = destination_file_path.argument();
	FileInfo input_info = File::get_info(input_path);
	FileInfo output_info = File::get_info(output_path);

	if( output_info.is_directory() ){
		output_path << "/" << FileInfo::base_name(input_path);
	} else {
		output_path = FileInfo::no_suffix(output_path);
	}
	output_path
			<<	"-" +
					String().format("%d", point_size()) +
					".sbi";

	m_icon_generator.set_bits_per_pixel( bits_per_pixel() );

	var::Vector<String> input_list;

	if( input_info.is_file() ){
		input_list.push_back(input_path);
	} else {
		input_list = Dir::read_list(input_path);
		for(auto & input: input_list){
			input = input_path + "/" + input;
		}
	}

	for(const auto & input: input_list){
		printer().debug("process icon " + input);
		process_icon(input);
	}

	printer().debug("generate icon file " + output_path);
	m_icon_generator.generate_icon_file(output_path);

	return 0;
}


int ConvertManager::process_font(
		File::SourcePath source_file_path,
		File::DestinationPath destination_directory_path
		){

	m_font_path = source_file_path.argument();
	m_bmp_font_generator.clear();
	m_bmp_font_generator.set_bits_per_pixel(bits_per_pixel());
	m_bmp_font_generator.set_generate_map(is_generate_map());

	//convert each character in the character set
	Dir::create(temporary_directory(), Permissions(0777));

	for(const auto & c: character_set()){
		process_character(c);
	}

	String output_path;
	output_path =
			destination_directory_path.argument() +
			"/" +
			FileInfo::base_name(m_font_path) +
			"-" +
			String().format("%d", m_point_size) +
			".sbf";


	String map_path = FileInfo::no_suffix(output_path) + map_suffix();
	m_bmp_font_generator.set_map_output_file(map_path);

	m_bmp_font_generator.generate_font_file(
				output_path
				);


	return 0;
}

int ConvertManager::process_icon(const var::String & path){
	String name = FileInfo::name(path);

	if( generate_icon_bitmap(path) < 0 ){
		return -1;
	}

	if( generate_icon_details(name) < 0 ){
		return -1;
	}

	return 0;
}
int ConvertManager::generate_icon_bitmap(const var::String & path){
	String name = FileInfo::name(path);
	String command;

	JsonObject svg_object = JsonDocument().load(
				JsonDocument::XmlFilePath(path)
				).to_object().at("svg").to_object();

	printer().open_object("svg", Printer::level_debug);
	printer() << svg_object;
	printer().close_object();

	Area view_box;
	{
		String view_box_string = svg_object.at("@viewBox").to_string();
		printer().debug("view box string is " + view_box_string);
		var::Vector<String> view_box_elements = view_box_string.split(" ");
		if(view_box_elements.count() != 4 ){
			printer().error("bad view box count %d", view_box_elements.count());
			return -1;
		}
		view_box.set_width(
					view_box_elements.at(2).to_integer() -
					view_box_elements.at(0).to_integer()
					);

		view_box.set_height(
					view_box_elements.at(3).to_integer() -
					view_box_elements.at(1).to_integer()
					);

		printer().open_object("viewBox");
		printer() << view_box;
		printer().close_object();
		sg_size_t max = view_box.maximum_dimension();

		m_density.format("%d", m_point_size * 100 / max);
		printer().key("density", m_density);
	}


	command =
			"convert -type truecolor -density " +
			m_density +
			" " +
			path +
			" -trim -negate " +
			temporary_icon_path(name);


	printer().debug(command);

	system(command.cstring());


	return 0;
}

int ConvertManager::generate_icon_details(const var::String & name){
	Bmp bmp(temporary_icon_path(name));

	Bitmap bitmap = bmp.convert_to_bitmap(
				Bitmap::BitsPerPixel( bits_per_pixel() )
				);

	printer().open_object(name, Printer::level_debug);

	String icon_name = FileInfo::base_name(name);
	printer().key("name", icon_name);
	sanitize_bitmap(bitmap);
	printer() << bitmap;
	sg_font_icon_t icon = {0};
	icon.id = 0;
	strncpy(
				icon.name,
				icon_name.cstring(),
				SG_FONT_ICON_MAX_NAME_LENGTH
				);
	printer().close_object();

	m_icon_generator.add_icon(
				IconGeneratorIcon(bitmap, icon)
				);

	return 0;
}

int ConvertManager::process_character(u16 unicode){


	//use "convert" to create a bmp for each character in the set
	if( generate_character_bitmap(unicode) < 0 ){
		return -1;
	}

	if( generate_character_details(unicode) < 0 ){
		return -1;
	}

	//import the BMP to a sgfx::Bitmap
	return 0;
}

int ConvertManager::generate_character_bitmap(u16 unicode){

	File unicode_text;

	if( unicode_text.create(
				temporary_unicode_text(),
				File::IsOverwrite(true)
				) < 0 ){
		return -1;
	}

	if( (unicode & 0xff) == unicode ){
		u8 byte = unicode;
		unicode_text.write(byte);
	} else {
		unicode_text.write(unicode);
	}
	unicode_text.close();

	//use system() to launch convert
	String command;

	command =
			"convert -background black -fill white -type truecolor -font " +
			m_font_path +
			" -pointsize " +
			String().format("%d", point_size()) +
			" label:@\"" +
			temporary_unicode_text() +
			"\" " +
			unicode_bitmap_path(unicode);

	printer().debug(command);

	system(command.cstring());

	return 0;
}

int ConvertManager::generate_character_details(u16 unicode){
	Bmp bmp(unicode_bitmap_path(unicode));

	Bitmap bitmap = bmp.convert_to_bitmap(
				Bitmap::BitsPerPixel( bits_per_pixel() )
				);

	printer().open_object(String().format("0x%04X", unicode), Printer::level_debug);
	printer() << bitmap;

	sg_font_char_t font_character = {0};
	Region active_region = bitmap.calculate_active_region();
	font_character.id = unicode;
	font_character.width = active_region.width();
	font_character.height = active_region.height();
	font_character.advance_x = font_character.width+1;
	font_character.offset_x = 0;
	font_character.offset_y = active_region.y();

	Bitmap active_bitmap(
				active_region.area(),
				Bitmap::BitsPerPixel(bits_per_pixel())
				);

	active_bitmap.draw_sub_bitmap(
				Point(0,0),
				bitmap,
				active_region.point(),
				active_region.area()
				);

	sanitize_bitmap(active_bitmap);

	printer() << active_bitmap;

	printer().close_object();



	m_bmp_font_generator.character_list().push_back(
				BmpFontGeneratorCharacter(
					active_bitmap,
					font_character
					)
				);


	return 0;
}

void ConvertManager::sanitize_bitmap(Bitmap & bitmap){
	sg_color_t max_color = (1<<bitmap.bits_per_pixel()) -1;
	//reserve a color for the border (max for bits per pixel)
	bitmap << Pen().set_color(max_color-1);
	for(sg_int_t x = 0; x < bitmap.width(); x++){
		for(sg_int_t y = 0; y < bitmap.height(); y++){

			sg_color_t color = bitmap.get_pixel(
						Point(x,y)
						);

			if( color == max_color ){
				bitmap.draw_pixel(
							Point(x,y)
							);
			}
		}
	}
}

