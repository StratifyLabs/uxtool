#include "BmpFontGenerator.hpp"

BmpFontGenerator::BmpFontGenerator(){
	m_is_ascii = false;
}

int BmpFontGenerator::generate_font_file(const String & destination){
	File font_file;
	u32 max_character_width = 0;
	u32 area;
	bool is_destination_valid = destination.is_empty() == false;
	
	String output_name = destination;
	if( FileInfo::suffix(destination).is_empty() ){
		output_name << ".sbf";
	}
	
	printer().info("Create output font file '%s'", output_name.cstring());

	if( is_destination_valid ){
		if( font_file.create(output_name, File::IsOverwrite(true)) <  0 ){
			printer().error("failed to create bmp font " + output_name);
			return -1;
		}
	}
	
	//populate the header
	sg_font_header_t header;
	area = 0;
	header.max_height = 0;
	sg_size_t min_offset_x = 65535, min_offset_y = 65535;
	
	for(u32 i = 0; i < character_list().count(); i++){
		if( character_list().at(i).font_character().width > max_character_width ){
			max_character_width = character_list().at(i).font_character().width;
		}
		
		if( character_list().at(i).font_character().height > header.max_height ){
			header.max_height = character_list().at(i).font_character().height;
		}
		
		if( character_list().at(i).font_character().offset_x < min_offset_x ){
			min_offset_x = character_list().at(i).font_character().offset_x;
		}
		
		if( character_list().at(i).font_character().offset_y < min_offset_y ){
			min_offset_y = character_list().at(i).font_character().offset_y;
		}
		
		area += (character_list().at(i).font_character().width*character_list().at(i).font_character().height);
	}
	
	printer().debug("Max width is %d", max_character_width);
	printer().debug("Max height is %d", header.max_height);
	
	for(u32 i=0; i < character_list().count(); i++){
		character_list().at(i).font_character().advance_x = character_list().at(i).font_character().width+1;
		character_list().at(i).font_character().offset_x = 0;
		character_list().at(i).font_character().offset_y -= min_offset_y;
	}
	
	
	header.version = SG_FONT_VERSION;
	header.bits_per_pixel = bits_per_pixel();
	header.max_word_width = sg_calc_word_width(max_character_width);
	header.character_count = character_list().count();
	header.kerning_pair_count = kerning_pair_list().count();
	header.size = sizeof(header) + header.character_count*sizeof(sg_font_char_t) + header.kerning_pair_count*sizeof(sg_font_kerning_pair_t);
	header.canvas_width = header.max_word_width*2*32;
	header.canvas_height = header.max_height*3/2;
	
	var::Vector<Bitmap> master_canvas_list = build_master_canvas(header);
	if( master_canvas_list.count() == 0 ){
		return -1;
	}
	
	for(u32 i=0; i < master_canvas_list.count(); i++){
		printer().open_object(String().format("master canvas %d", i), Printer::DEBUG);
		printer() << master_canvas_list.at(i);
		printer().close_object();
	}
	
	
	printer().info("header bits_per_pixel %d", header.bits_per_pixel);
	printer().info("header max_word_width %d", header.max_word_width);
	printer().info("header character_count %d", header.character_count);
	printer().info("header kerning_pair_count %d", header.kerning_pair_count);
	printer().info("header size %d", header.size);
	printer().info("header canvas_width %d", header.canvas_width);
	printer().info("header canvas_height %d", header.canvas_height);
	
	printer().debug(
				"write header to file at %d (size:%d)",
				font_file.location(),
				sizeof(header)
				);
	if( is_destination_valid && font_file.write(header) != sizeof(header) ){
		printer().error(
					"failed to write header to file (%d, %d)",
					font_file.return_value(),
					font_file.error_number()
					);
		return -1;
	}

	printer().debug(
				"write kerning pairs to file at %d",
				font_file.location()
				);
	
	printer().open_array("kerning pairs", Printer::DEBUG);
	
	for(u32 i = 0; i < kerning_pair_list().count(); i++){
		Data kerning_pair;
		kerning_pair.refer_to(kerning_pair_list().at(i));
		printer().debug("kerning %d -> %d -- %d",
										kerning_pair_list().at(i).unicode_first,
										kerning_pair_list().at(i).unicode_second,
										kerning_pair_list().at(i).horizontal_kerning);
		if( is_destination_valid && font_file.write(kerning_pair_list().at(i)) != sizeof(sg_font_kerning_pair_t) ){
			printer().error(
						"failed to write kerning pair (%d,%d)",
						font_file.return_value(),
						font_file.error_number()
						);
			return -1;
		}
	}
	printer().close_array();

	printer().debug("sort character list");
	character_list().sort(
				var::Vector<BmpFontGeneratorCharacter>::ascending
	);

	printer().debug(
				"write characters defs to file at %d",
				font_file.location()
				);
	
	//write characters in order
	//for(u32 i = 0; i < 65535; i++){
	for(u32 j = 0; j < character_list().count(); j++){
		//if( character_list().at(j).id == i ){
		printer().debug("write character %d to file on %d: %d,%d %dx%d at %d",
										character_list().at(j).font_character().id,
										character_list().at(j).font_character().canvas_idx,
										character_list().at(j).font_character().canvas_x,
										character_list().at(j).font_character().canvas_y,
										character_list().at(j).font_character().width,
										character_list().at(j).font_character().height,
										font_file.seek(0, File::CURRENT));
		if( is_destination_valid &&
				font_file.write(
					character_list().at(j).font_character()
					) != sizeof(sg_font_char_t) ){
			printer().error("failed to write kerning pair");
			return -1;
		}
		//break;
		//}
	}
	//}
	
	//write the master canvas
	printer().debug(
				"write master canvas to file at %d",
				font_file.location()
				);


	for(u32 i=0; i < master_canvas_list.count(); i++){
		printer().debug("write master canvas %d (%d) to file at %d",
										i,
										master_canvas_list.at(i).size(),
										font_file.seek(0, File::CURRENT)
										);
		if( is_destination_valid &&
				font_file.write(
					master_canvas_list.at(i)
					) != (int)master_canvas_list.at(i).size() ){
			printer().error("Failed to write master canvas %d", i);
			return -1;

		}
	}

	if( is_generate_map() ){
		printer().info("generate map file '%s'", m_map_output_file.cstring());
		generate_map_file(header, master_canvas_list);
	}
	
	font_file.close();
	
	return 0;
}

int BmpFontGenerator::generate_map_file(
		const sg_font_header_t & header,
		const var::Vector<Bitmap> & master_canvas_list
		){

	JsonObject map_object;
	JsonObject header_object;
	JsonArray kerning_pair_array;
	JsonArray character_array;

	header_object
			.insert("version", JsonString( String().format("0x%04X", header.version)) );
	header_object.insert("size", JsonInteger(header.size));
	header_object.insert("characterCount", JsonInteger(header.character_count));
	header_object.insert("maxWordWidth", JsonInteger(header.max_word_width));
	header_object.insert("bitsPerPixel", JsonInteger(header.bits_per_pixel));
	header_object.insert("canvasWidth", JsonInteger(header.canvas_width));
	header_object.insert("canvasHeight", JsonInteger(header.canvas_height));
	header_object.insert("kerningPairCount", JsonInteger(header.kerning_pair_count));
	header_object.insert("maxHeight", JsonInteger(header.max_height));


	printer().info("create map with %d bits per pixel", master_canvas_list.at(0).bits_per_pixel());

	for(u32 i=0; i < kerning_pair_list().count(); i++){
		JsonObject kerning_object;
		kerning_object.insert(
					"unicodeFirst",
					JsonInteger(kerning_pair_list().at(i).unicode_first)
					);
		kerning_object.insert(
					"unicodeSecond",
					JsonInteger(kerning_pair_list().at(i).unicode_second)
					);

		kerning_object.insert(
					"horizontalKerning",
					JsonInteger(kerning_pair_list().at(i).horizontal_kerning)
					);

		kerning_pair_array.append(kerning_object);
	}

	for(u32 i=0; i < character_list().count(); i++){

		JsonObject character_object;
		character_object.insert(
					"id",
					JsonInteger(character_list().at(i).font_character().id)
					);
		character_object.insert(
					"width",
					JsonInteger(character_list().at(i).font_character().width)
					);
		character_object.insert(
					"height",
					JsonInteger(character_list().at(i).font_character().height)
					);
		character_object.insert(
					"advanceX",
					JsonInteger(character_list().at(i).font_character().advance_x)
					);
		character_object.insert(
					"offsetX",
					JsonInteger(character_list().at(i).font_character().offset_x)
					);
		character_object.insert(
					"offsetY",
					JsonInteger(character_list().at(i).font_character().offset_y)
					);
		character_object.insert(
					"canvasIndex",
					JsonInteger(character_list().at(i).font_character().canvas_idx)
					);
		character_object.insert(
					"canvasX",
					JsonInteger(character_list().at(i).font_character().canvas_x)
					);
		character_object.insert(
					"canvasY",
					JsonInteger(character_list().at(i).font_character().canvas_y)
					);

		character_object.insert(
					"linePadding",
					JsonInteger(2)
					);

		Bitmap canvas(
					Area(character_list().at(i).font_character().width,
							 character_list().at(i).font_character().height),
					Bitmap::BitsPerPixel(bits_per_pixel())
					);
		canvas.clear();
		canvas.draw_sub_bitmap(
					Point(),
					master_canvas_list.at(
						character_list().at(i).font_character().canvas_idx),
					Region(
						Point(character_list().at(i).font_character().canvas_x,
									character_list().at(i).font_character().canvas_y),
						canvas.area()
						));
		String pixel_character;
		JsonArray character_lines_array;
		for(sg_size_t h = 0; h < canvas.height(); h++){
			String line = " ";
			for(sg_size_t w = 0; w < canvas.width(); w++){
				line << Printer::get_bitmap_pixel_character(
									canvas.get_pixel(Point(w,h)),
									bits_per_pixel()
									);
				pixel_character.format("%c", Printer::get_bitmap_pixel_character(canvas.get_pixel(Point(w,h)), bits_per_pixel()));
			}
			line << " ";
			character_lines_array.append(JsonString(line));
		}
		character_object.insert(
					"lines",
					character_lines_array
					);

		character_array.append(character_object);
	}

	map_object.insert("header", header_object);
	map_object.insert("kerningPairs", kerning_pair_array);
	map_object.insert("characters", character_array);

	if( JsonDocument().save(
				map_object,
				File::Path(m_map_output_file)
				) < 0 ){
		printer().error(
					"failed to save map json file as %s",
					m_map_output_file.cstring()
					);
	}

	return 0;
}

var::Vector<Bitmap> BmpFontGenerator::build_master_canvas(
		const sg_font_header_t & header
		){
	var::Vector<Bitmap> master_canvas_list;
	Bitmap master_canvas;
	
	printer().info("max word width %d", header.max_word_width);
	
	Area master_dim(header.canvas_width, header.canvas_height);
	
	printer().info("master width %d", master_dim.width());
	printer().info("master height %d", master_dim.height());
	
	if( master_canvas.allocate(
				master_dim,
				Bitmap::BitsPerPixel(header.bits_per_pixel)
				) < 0 ){
		printer().error("Failed to allocate memory for master canvas");
		return master_canvas_list;
	}
	master_canvas.clear();
	
	
	for(u32 i = 0; i < character_list().count(); i++){
		//find a place for the character on the master bitmap
		Region region;
		Area character_dim(
					character_list().at(i).font_character().width,
					character_list().at(i).font_character().height
					);
		
		if( character_dim.width() && character_dim.height() ){
			do {
				
				for(u32 j = 0; j < master_canvas_list.count(); j++){
					region = find_space_on_canvas(
								master_canvas_list.at(j),
								character_list().at(i).bitmap().area()
								);
					if( region.is_valid() ){
						printer().debug("allocate %d (%c) to master canvas %d at %d,%d",
														character_list().at(i).font_character().id,
														character_list().at(i).font_character().id,
														j,
														region.x(), region.y());
						character_list().at(i).font_character().canvas_x = region.x();
						character_list().at(i).font_character().canvas_y = region.y();
						character_list().at(i).font_character().canvas_idx = j;
						break;
					}
				}
				
				if( region.is_valid() == false ){
					master_canvas_list.push_back(master_canvas);
				}
				
			} while( !region.is_valid() );
		}
	}
	
	for(u32 i = 0; i < character_list().count(); i++){
		Point p(
					character_list().at(i).font_character().canvas_x,
					character_list().at(i).font_character().canvas_y
					);
		master_canvas_list.at(
					character_list().at(i).font_character().canvas_idx
					).draw_bitmap(p, character_list().at(i).bitmap());
	}
	
	return master_canvas_list;
}


int BmpFontGenerator::import_map(const String & map){
	File map_file;
	String path;
	
	path = map;

	JsonObject map_object = JsonDocument().load(
				File::Path(path)
				).to_object();

	if( map_object.is_empty() ){
		printer().error("Failed to open map file %s", path.cstring());
		return -1;
	}
	
	sg_font_header_t header;
	JsonObject header_object
			= map_object.at("header").to_object();
	JsonArray kerning_pair_array
			= map_object.at("kerningPairs").to_array();
	JsonArray character_array
			= map_object.at("characters").to_array();

	if( header_object.is_empty() || character_array.is_empty() ){
		printer().error(
					"failed to load font from map file %s",
					path.cstring()
					);
		return -1;
	}

	header.version = header_object.at("version").to_string().to_unsigned_long(String::base_16);
	header.size = header_object.at("size").to_integer();
	header.character_count = header_object.at("characterCount").to_integer();
	header.max_height = header_object.at("maxHeight").to_integer();
	header.max_word_width = header_object.at("maxWordWidth").to_integer();
	header.bits_per_pixel = header_object.at("bitsPerPixel").to_integer();
	header.kerning_pair_count = header_object.at("kerningPairCount").to_integer();
	header.canvas_height = header_object.at("canvasHeight").to_integer();
	header.canvas_width = header_object.at("canvasWidth").to_integer();

	set_bits_per_pixel(header.bits_per_pixel);


	for(u32 i=0; i < kerning_pair_array.count(); i++){
		JsonObject kerning_object = kerning_pair_array.at(i).to_object();
		sg_font_kerning_pair_t kerning_pair;

		kerning_pair.unicode_first = kerning_object.at("unicodeFirst").to_integer();
		kerning_pair.unicode_second = kerning_object.at("unicodeSecond").to_integer();
		kerning_pair.horizontal_kerning = kerning_object.at("horizontalKerning").to_integer();

		m_kerning_pair_list.push_back(kerning_pair);
	}



	for(u32 i=0; i < character_array.count(); i++){
		JsonObject character_object = character_array.at(i).to_object();
		sg_font_char_t font_character;

		font_character.id = character_object.at("id").to_integer();
		font_character.canvas_x = character_object.at("canvasX").to_integer();
		font_character.canvas_y = character_object.at("canvasY").to_integer();
		font_character.canvas_idx = character_object.at("canvasIndex").to_integer();
		font_character.width = character_object.at("width").to_integer();
		font_character.height = character_object.at("height").to_integer();
		font_character.advance_x = character_object.at("advanceX").to_integer();
		font_character.offset_x = character_object.at("offsetX").to_integer();
		font_character.offset_y = character_object.at("offsetY").to_integer();

		u32 line_padding = character_object.at("linePadding").to_integer();
		JsonArray character_lines_array = character_object.at("lines").to_array();
		Bitmap character_bitmap(
					Area(font_character.width, font_character.height),
					Bitmap::BitsPerPixel(1)
					);
		character_bitmap.clear();

		for(u32 j=0; j < character_lines_array.count(); j++){
			String line;
			line = character_lines_array.at(j).to_string();
			line = line.create_sub_string(
						String::Position(line_padding/2),
						String::Length(line.length() - line_padding)
						);
			for(u32 k=0; k < line.length(); k++){
				sg_color_t color = Printer::get_bitmap_pixel_color(line.at(k), header.bits_per_pixel);

				character_bitmap.set_pen( Pen().set_color(color) );

				character_bitmap.draw_pixel(
							Point(k,j)
							);
			}
		}

		m_character_list.push_back(
					BmpFontGeneratorCharacter(
						character_bitmap,
						font_character
						)
					);
	}

	return 0;
}
