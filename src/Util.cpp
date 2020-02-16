/*! \file */ //Copyright 2011-2018 Tyler Gilbert; All Rights Reserved

#include <cmath>
#include <sapi/fmt.hpp>
#include <sapi/fs.hpp>
#include "Util.hpp"

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
		if( f.read(header) != sizeof(sg_font_header_t) ){
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


		Vector<sg_font_kerning_pair_t> kerning_pairs;
		for(u32 i=0; i < header.kerning_pair_count; i++){
			sg_font_kerning_pair_t pair;
			if( f.read(pair) != sizeof(sg_font_kerning_pair_t) ){
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

		Vector<sg_font_char_t> characters;
		for(u32 i=0; i < header.character_count; i++){
			sg_font_char_t character;
			printer().debug("read character from %d", f.seek(0, File::CURRENT));
			if( f.read(character) != sizeof(sg_font_char_t) ){
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

		u32 length = ff.calculate_length(
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
