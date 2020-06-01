/*! \file */ //Copyright 2011-2018 Tyler Gilbert; All Rights Reserved

#include <cmath>
#include <sapi/fmt.hpp>
#include <sapi/fs.hpp>
#include "Util.hpp"


bool Util::convert(const Options& options){
	if( options.is_json_printer() && (options.input_suffix() == "json") ){
		printer().debug("converting JSON to BMP");
		convert_json_to_bmp(options);
		return true;
	}

	PRINTER_TRACE(
				printer(),
				String().format(
					"not converting using UTIL from JSON printer input (%d %d)",
					options.is_json_printer(),
					options.input_suffix() == "json"
					)
				);
	return false;
}

void Util::show(const Options& options){
	printer().message("action=show");

	if( options.input_suffix() == "svic" ){
		printer().message(
					"Show icon file %s with canvas size %d",
					options.input().cstring(),
					options.point_size().to_integer()
					);

		if( options.output().is_empty() == false ){
			if( options.output_suffix() != "bmp" ){
				Ap::printer().error(
							"output must be a .bmp file"
							);
			}
		}

		show_icon_file(
					File::SourcePath(options.input()),
					File::DestinationPath(options.output()),
					options.point_size().to_integer(),
					options.downsample_size().to_integer(),
					options.bits_per_pixel().to_integer()
					);
	} else if( options.input_suffix() == "sbi" ){

		Ap::printer().message("show icon font " + options.input());
		show_icon_font(
					File::SourcePath(options.input()),
					File::DestinationPath(options.output()),
					Bitmap::BitsPerPixel(options.bits_per_pixel().to_integer()),
					Util::IsDetails(options.is_details())
					);

	} else {
		printer().message(
					"Show font file %s with details %d",
					options.input().cstring(),
					options.is_details()
					);
		show_file_font(
					File::SourcePath(options.input()),
					File::DestinationPath(options.output()),
					Bitmap::BitsPerPixel(options.bits_per_pixel().to_integer()),
					Util::IsDetails(options.is_details())
					);
	}
	exit(0);
}

void Util::convert_json_to_bmp(const Options& options){
	JsonObject image = JsonDocument().load(
				JsonDocument::FilePath(options.input())
				);

	if( image.is_empty() ){
		printer().error("failed to load JSON object from " + options.input());
		return;
	}


	StringList keys = image.keys();
	if( keys.find("line-0000") == keys.count() ){
		printer().error("JSON object does not contain a printer image");
		return;
	}

	var::Vector<String> line_list;

	for(u32 i=0; i < keys.count(); i++){
		String key = String::number(i, "line-%04d");
		if( image.at(key).is_string() ){
			line_list.push_back( image.at(key).to_string() );
		}
	}

	int height = line_list.count();
	if( height <= 0 ){
		printer().error("Image has zero height");
		return;
	}

	int width = line_list.at(0).length() - 2;

	if( width <= 1 ){
		printer().error("Image has zero width");
		return;
	}

	Bitmap bitmap(
				Area(width, height),
				Bitmap::BitsPerPixel(options.bits_per_pixel().to_integer())
				);

	for(int h = 0; h < height; h++){
		for(int w =	0; w < width; w++){
			const String& line = line_list.at(h);
			char pixel_value = ' ';
			if( line.length() > w+1 ){
				pixel_value = line_list.at(h).at(w+1);
			}
			bitmap << Pen().set_color(
									Printer::get_bitmap_pixel_color(
										pixel_value,
										options.bits_per_pixel().to_integer()
										)
									);
			bitmap.draw_pixel(Point(w,h));
		}
	}

	String output = options.output_file_path() + ".bmp";


	printer().info("creating " + output);
	printer() << bitmap;
	if( Bmp::save(
				output,
				bitmap,
				Palette()
				.set_color_count(Palette::color_count_1bpp)
				.set_pixel_format(Palette::pixel_format_rgb888)
				.assign_color(0, PaletteColor("#ffffff"))
				.assign_color(1, PaletteColor("#671d86"))
				) < 0 ){
		printer().error("failed to save image to " + output);
	}
}


void Util::filter(
		Bitmap & bitmap
		){


	sg_antialias_filter_t filter;
	u8 contrast_data[8];

	contrast_data[0] = 0;
	contrast_data[1] = 0;
	contrast_data[2] = 1;
	contrast_data[3] = 1;
	contrast_data[4] = 2;
	contrast_data[5] = 2;
	contrast_data[6] = 2;
	contrast_data[7] = 2;

	bitmap.api()->antialias_filter_init(
				&filter,
				contrast_data);

	bitmap.api()->antialias_filter_apply(
				bitmap.bmap(),
				&filter,
				Region(
					Point(0,0),
					Area(bitmap.area())
					)
				);
}

void Util::show_icon_file(
		File::SourcePath input_file,
		File::DestinationPath output_file,
		sg_size_t canvas_size,
		u16 downsample_size,
		u8 bits_per_pixel
		){

	File icon_file;
	YamlPrinter p;
	bool is_write_bmp = false;
	Bitmap bmp_output;
	if( !output_file.argument().is_empty() ){
		bmp_output.set_bits_per_pixel(bits_per_pixel);
		is_write_bmp = true;
	}

	if( icon_file.open(
				input_file.argument(),
				OpenFlags::read_only()
				) < 0 ){
		printf("failed to open vector icon file\n");
		return;
	}

	Svic icon_collection(input_file.argument());
	Bitmap canvas(
				Area(canvas_size, canvas_size),
				Bitmap::BitsPerPixel(1)
				);

	canvas.set_bits_per_pixel(bits_per_pixel);

	Area downsampled_area(
				downsample_size, downsample_size
				);

	Area canvas_downsampled_area;
	canvas_downsampled_area.set_width( (canvas.width() + downsample_size/2) / downsample_size );
	canvas_downsampled_area.set_height( (canvas.height() + downsample_size/2) / downsample_size );
	Bitmap canvas_downsampled(
				canvas_downsampled_area,
				Bitmap::BitsPerPixel(1)
				);
	canvas_downsampled.set_bits_per_pixel(bits_per_pixel);

	p.message("%d icons in collection", icon_collection.count());

	u32 count = (sqrtf(icon_collection.count()) + 0.5f);

	if( is_write_bmp ){
		bmp_output.allocate(
					Area(
						canvas_downsampled_area.width() * count,
						canvas_downsampled_area.height() * count
						)
					);
		bmp_output.clear();
	}

	for(u32 i=0; i < icon_collection.count(); i++){

		p.key("name", icon_collection.name_at(i));
		VectorPath vector_path = icon_collection.at(i);
		vector_path << canvas.get_viewable_region();
		canvas.clear();
		canvas.set_pen( Pen().set_color((u32)-1) );

		VectorMap map;
		map.calculate_for_bitmap(canvas);

		sgfx::Vector::draw(
					canvas,
					vector_path,
					map
					);

		canvas_downsampled.downsample_bitmap(
					canvas,
					downsampled_area
					);

		//filter(canvas_downsampled);
		p.open_object("icon") << canvas_downsampled;
		p.close_object();

		if( is_write_bmp ){
			bmp_output.draw_sub_bitmap(
						Point(
							(i % count) * canvas_downsampled_area.width(),
							i / count * canvas_downsampled_area.height()
							),
						canvas_downsampled,
						Region(Point(0,0), canvas_downsampled_area)
						);
		}

	}

	if( is_write_bmp ){

		Palette bmp_pallete;

		bmp_pallete
				.set_color_count(
					Palette::get_color_count(bits_per_pixel)
					)
				.create_gradient(PaletteColor(0xffffffff));


		fmt::Bmp::save(
					output_file.argument(),
					bmp_output,
					bmp_pallete
					);
	}

}


void Util::clean_path(const String & path, const String & suffix){
	Dir d;
	const char * entry;
	String str;

	if( d.open(path) < 0 ){
		printf("Failed to open path: '%s'\n", path.cstring());
	}

	while( (entry = d.read()) != 0 ){
		str = File::suffix(entry);
		if( str == "sbf" ){
			str.clear();
			str << path << "/" << entry;
			printf("Removing file: %s\n", str.cstring());
			File::remove(str);
		}
	}
}

void Util::show_file_font(
		File::SourcePath input_file,
		File::DestinationPath output_file,
		Bitmap::BitsPerPixel bpp,
		IsDetails is_details
		){

	FontInfo font_info(input_file.argument());
	Ap::printer().info("Show font %s", input_file.argument().cstring());

	font_info.create_font();

	Font & ff = *(font_info.font());
	show_font(ff, bpp);

	if( is_details.argument() ){
		File f;

		if( f.open(
					input_file.argument(),
					OpenFlags::read_only()
					) < 0 ){
			Ap::printer().error(
						"Failed to open file '%s'",
						input_file.argument().cstring()
						);
			return;
		}

		sg_font_header_t header;
		if( f.read(
					Reference(header)
					) != sizeof(sg_font_header_t) ){
			Ap::printer().error("failed to read header");
			return;
		}

		printer().open_object("header");
		{
			printer().key("version", "0x%04X", header.version);
			printer().key("size", "%d", header.size);
			printer().key("characterCount", "%d", header.character_count);
			printer().key("kerningPairCount", "%d", header.kerning_pair_count);
			printer().key("maxWordWidth", "%d", header.max_word_width);
			printer().key("maxHeight", "%d", header.max_height);
			printer().key("bitsPerPixel", "%d", header.bits_per_pixel);
			printer().key("canvasWidth", "%d", header.canvas_width);
			printer().key("canvasHeight", "%d", header.canvas_height);
			printer().close_object();
		}


		var::Vector<sg_font_kerning_pair_t> kerning_pairs;
		for(u32 i=0; i < header.kerning_pair_count; i++){
			sg_font_kerning_pair_t pair;
			if( f.read(
						Reference(pair)
						) != sizeof(sg_font_kerning_pair_t) ){
				Ap::printer().error("Failed to read kerning pair");
				return;
			}
			kerning_pairs.push_back(pair);
		}

		if( header.kerning_pair_count > 0 ){
			Ap::printer().open_array("kerning pairs");
			for(u32 i=0; i < kerning_pairs.count(); i++){
				Ap::printer().key("kerning", "%d %d %d",
													kerning_pairs.at(i).unicode_first,
													kerning_pairs.at(i).unicode_second,
													kerning_pairs.at(i).horizontal_kerning);

			}
			Ap::printer().close_array();
		} else {
			Ap::printer().info("no kerning pairs present");
		}

		var::Vector<sg_font_char_t> characters;
		for(u32 i=0; i < header.character_count; i++){
			sg_font_char_t character;
			printer().debug("read character from %d", f.seek(0, File::whence_current));
			if( f.read(
						Reference(character)
						) != sizeof(sg_font_char_t) ){
				Ap::printer().error(
							"Failed to read character at %d (%d, %d)",
							i,
							f.return_value(),
							f.error_number()
							);
				continue;
			}
			printer().debug("push character %d", character.id);
			characters.push_back(character);
		}

		Ap::printer().open_object("characters");
		for(u32 i=0; i < characters.count(); i++){
			Ap::printer().key(String().format("%d", characters.at(i).id), "canvas %d->%d,%d %dx%d advancex->%d offset->%d,%d",
												characters.at(i).canvas_idx,
												characters.at(i).canvas_x,
												characters.at(i).canvas_y,
												characters.at(i).width,
												characters.at(i).height,
												characters.at(i).canvas_x,
												characters.at(i).advance_x,
												characters.at(i).offset_x,
												characters.at(i).offset_y);

		}
		Ap::printer().close_array();
	}


	if( output_file.argument().is_empty() == false ){
		ff.set_space_size(8);

		String ascii_characters;
		for(char c: String(ff.ascii_character_set())){
			ascii_characters << c << " ";
		}

		u32 length = ff.get_width(
					ascii_characters
					);

		Bitmap output_bitmap(
					Area(
						length+4,
						ff.get_height()
						),
					bpp
					);

		output_bitmap << Pen().set_color(
											 (1 << bpp.argument()) - (1<<ff.bits_per_pixel())
											 ).set_zero_transparent();

		ff.draw(
					ascii_characters,
					output_bitmap,
					Point(2,0)
					);

		printer().debug("apply filter");


		Bmp::save(
					output_file.argument(),
					output_bitmap,
					Palette()
					.set_color_count(
						Palette::get_color_count(bpp.argument())
						)
					.create_gradient(PaletteColor(0xffffffff))
					);

	}

}

void Util::show_icon_font(
		File::SourcePath input_file,
		File::DestinationPath output_file,
		Bitmap::BitsPerPixel bpp,
		IsDetails is_details
		){

	File icon_file;
	if( icon_file.open(
				input_file.argument(),
				OpenFlags::read_only()) < 0 ){
		printer().error("failed to open " + input_file.argument());
		return;
	}

	IconFont icon_font(icon_file);

	printer().open_object("header");
	printer().key("count", "%d", icon_font.count());
	printer().key("maxWidth", "%d", icon_font.area().width());
	printer().key("maxHeight", "%d", icon_font.area().height());
	printer().close_object();

	Bitmap destination(
				icon_font.area(),
				bpp
				);

	for(size_t i = 0; i < icon_font.count(); i++){
		destination.fill(0);


		IconInfo info = icon_font.get_info(i);
		Point destination_point(
					(destination.width() - info.area().width())/2,
					(destination.height() - info.area().height())/2
					);

		icon_font.draw(i, destination, destination_point);


		printer().open_object(info.name());
		if( is_details.argument() ){
			printer().key("canvasIdx", "%d", info.canvas_idx());
			printer() << info.area();
			printer() << info.canvas_point();
		}
		printer() << destination;
		printer().close_object();
	}

}

void Util::show_font(
		Font & f,
		Bitmap::BitsPerPixel bpp
		){
	Bitmap b;
	u32 i;

	Ap::printer().info(
				"Alloc bitmap %d x %d with %d bpp",
				f.get_width(),
				f.get_height(),
				bpp.argument()
				);

	b.allocate(
				Area(f.get_width()*8/4, f.get_height()*5/4),
				bpp
				);

	for(i=0; i < Font::ascii_character_set().length(); i++){
		b.clear();
		String string;
		string << Font::ascii_character_set().at(i);

		if( i < Font::ascii_character_set().length()-1 ){
			string << Font::ascii_character_set().at(i+1);
			Ap::printer().info("Character: %c", Font::ascii_character_set().at(i+1));
		} else {
			Ap::printer().info("Character: %c", Font::ascii_character_set().at(i));
		}
		b << Pen().set_color(1).set_blend();
		f.draw(string, b, Point(2, 0));
		Ap::printer().open_object("character info");
		Ap::printer().key("width", "%d", f.character().width);
		Ap::printer().key("height", "%d", f.character().height);
		Ap::printer().key("advance_x", "%d", f.character().advance_x);
		Ap::printer().key("offset_x", "%d", f.character().offset_x);
		Ap::printer().key("offset_y", "%d", f.character().offset_y);
		Ap::printer().key("canvas_idx", "%d", f.character().canvas_idx);
		Ap::printer().key("canvas_x", "%d", f.character().canvas_x);
		Ap::printer().key("canvas_y", "%d", f.character().canvas_y);
		Ap::printer().close_object();
		Ap::printer() << b;
	}
}

void Util::show_system_font(int idx){

#if defined __link
	printf("System fonts not available\n");

#else
	printf("Not implemented\n");
#endif
}
