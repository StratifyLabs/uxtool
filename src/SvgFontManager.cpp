/*! \file */ //Copyright 2011-2018 Tyler Gilbert; All Rights Reserved


#include <cmath>
#include <sapi/fmt.hpp>
#include <sapi/var.hpp>
#include <sapi/sys.hpp>
#include <sapi/chrono.hpp>
#include <sapi/hal.hpp>
#include <sapi/sgfx.hpp>
#include "SvgFontManager.hpp"


SvgFontManager::SvgFontManager() {
	// TODO Auto-generated constructor stub
	m_scale = 0.0f;
	m_is_show_canvas = true;
	m_bmp_font_generator.set_is_ascii();
	m_scale_sign_y = -1;
}

int SvgFontManager::process_icons(
		fs::File::SourcePath source_path,
		fs::File::DestinationPath destination_path
		){


	printer().open_object("svg.icons");
	var::Vector<String> input_files;
	String output_file_path;
	{
		String input_base_name;
		if( File::get_info(source_path.argument()).is_directory() ){
			input_base_name = FileInfo::name(source_path.argument());
			input_files = Dir::read_list(
						source_path.argument(),
						[](const String & entry)
						-> const String {
				return (FileInfo::suffix(entry) == "svg") ?
							entry :
							String();
			});

			for(auto & input: input_files){
				input = source_path.argument() + "/" + input;
			}

		} else {
			input_files.push_back(source_path.argument());
		}

		if( File::get_info(destination_path.argument()).is_directory() ){
			output_file_path
					<< destination_path.argument()
					<< "/"
					<< input_base_name
					<< ".svic";
		} else {
			output_file_path = destination_path.argument();
		}
	}

	Svic vector_collection;

	printer().debug(
				"create svic output file %s",
				output_file_path.cstring()
				);

	if( vector_collection.create(
				output_file_path,
				File::IsOverwrite(true)
				) < 0 ){
		printer().error(
					"Failed to create output file %s",
					output_file_path.cstring()
					);
		printer().close_object();
		return -1;
	}

	//for(u32 i=0; i < icon_array.count(); i++){
	sg_vector_icon_header_t header;
	memset(&header, 0, sizeof(header));

	for(auto const & input_file: input_files){
		printer().message(
					"process input file %s",
					input_file.cstring()
					);


		JsonDocument json_document;
		JsonObject top = json_document.load(
					JsonDocument::XmlFilePath(
						input_file
						)
					).to_object();

		if( m_is_output_json ){
			String json_output_path = FileInfo::no_suffix(input_file) + ".json";

			if( Reference( JsonDocument().stringify(top) ).save(
						json_output_path,
						Reference::IsOverwrite(true)
						) < 0 ){
				printer().error("Failed to save JSON version of file at " +
												json_output_path
												);
				return -1;
			} else {
				printer().info("JSON of SVG saved to " + json_output_path);
			}
		}

		JsonObject svg_icon = top.at("svg").to_object();

		if( !svg_icon.is_valid() || svg_icon.is_empty() ){
			printer().error(
						"failed to find svg icon in %s",
						source_path.argument().cstring()
						);
			printer().close_object();
			return -1;
		}


		process_svg_icon(svg_icon);

		if( m_vector_path_icon_list.count() > 0 ){
			printer().message("get name of icon from @data-icon");
			String name =
					svg_icon.at("@data-icon").to_string();

			if( name.is_empty() ){
				name = FileInfo::base_name(input_file);
			}

			printer().message(
						"add %s to vector collection (%d objects)",
						name.cstring(),
						m_vector_path_icon_list.count()
						);

			if( vector_collection.append(
						name,
						m_vector_path_icon_list
						) < 0 ){
				printer().error("Failed to add %s to vector collection", name.cstring());
			}
		}
	}

	printer().message("closing collection file");
	vector_collection.close();
	printer().close_object();

	return 0;
}

int SvgFontManager::process_svg_icon(
		const JsonObject & object
		){

	printer().open_object("svg.convert");
	printer().open_object("json", sys::Printer::DEBUG);
	{
		printer() << object;
		printer().close_object();
	}

	String view_box
			= object.at("@viewBox").to_string();

	printer().message(
				"viewBox %s",
				view_box.cstring()
				);

	if( view_box.is_empty() == false ){
		m_bounds = parse_bounds(view_box.cstring());
		m_aspect_ratio = m_bounds.width() * 1.0f / m_bounds.height();
		//m_aspect_ratio = 1.0f;
		m_canvas_dimensions = Area(m_canvas_size, m_canvas_size);
		printer().open_object("bounds") << m_bounds;
		printer().close_object();
		printer().open_object("Canvas Dimensions") << m_canvas_dimensions;
		printer().close_object();
	} else {
		printer().warning("Failed to find bounding box");
		printer().close_object();
		return -1;
	}

	m_scale = ( SG_MAP_MAX * 1.0f ) / (m_bounds.area().maximum_dimension());
	printer().message("Scaling factor is %0.2f", m_scale);

	//d is the path

	JsonObject path = object.at("path").to_object();
	if( path.is_empty() ){
		printer().error("path not found for icon");
		return -1;
	}

	String drawing_path;
	drawing_path = path.at("@d").to_string();
	if( drawing_path.is_empty() ){
		printer().error("drawing path not found for icon");
		return -1;
	}

	Bitmap canvas;
	m_vector_path_icon_list
			= convert_svg_path(
				canvas,
				drawing_path,
				m_canvas_dimensions,
				m_pour_grid_size,
				true
				);

	printer().open_object("canvas size") << canvas.area();
	printer().close_object();
	printer().open_object(
				"canvas",
				canvas.area().width() > 128 ?
					Printer::DEBUG :
					Printer::INFO
					);
	printer() << canvas;
	printer().close_object();

	printer().debug("finished icon");
	printer().close_object();

	return 0;
}

int SvgFontManager::process_font(
		fs::File::SourcePath source_file_path,
		fs::File::DestinationPath destination_directory_path
		){
	u32 j;

	JsonDocument json_document;

	//some docs start as an array
	JsonObject font_object =
			json_document.load(
				JsonDocument::XmlFilePath(source_file_path.argument())
				).to_object();

	if( m_is_output_json ){
		String json_output_path = FileInfo::no_suffix(source_file_path.argument()) + ".json";

		if( Reference( JsonDocument().stringify(font_object) ).save(
					json_output_path,
					Reference::IsOverwrite(true)
					) < 0 ){
			printer().error("Failed to save JSON version of file at " +
											json_output_path
											);
			return -1;
		} else {
			printer().info("JSON of SVG saved to " + json_output_path);
		}
	}


	JsonObject svg_object = font_object.at("svg").to_object();

	if( svg_object.is_empty() ){
		printer().debug(
					"loaded svg file %s",
					source_file_path.argument().cstring()
					);
		printer().error("no svg object found");
		return -1;
	}

	printer().open_object("svg.convert");

	String metadata = svg_object.at("metadata").to_string();
	JsonObject defs = svg_object.at("defs").to_object();
	JsonObject font = defs.at("font").to_object();
	JsonArray glyphs = font.at("glyph").to_array();
	JsonArray hkerns = font.at("hkern").to_array();
	JsonObject font_face = font.at("font-face").to_object();
	JsonObject missing_glyph = font.at("missing-glyph").to_object();

	printer().key("metadata", metadata);
	printer().key("font", font.at("@id").to_string());
	printer().key("fontFamily", font_face.at("@font-family").to_string());
	printer().key("fontStretch", font_face.at("@font-stretch").to_string());
	u16 units_per_em = font_face.at("@units-per-em").to_string().to_integer();

	printer().key("unitsPerEm", "%d", units_per_em);

	m_scale = (SG_MAP_MAX*1.0f) / (units_per_em);

	String value;
	value = font_face.at("@bbox").to_string();
	printer().message(
				"bbox: %s",
				value.cstring()
				);

	if( value.is_empty() ){
		printer().warning("Failed to find bounding box");
		return -1;
	}

	m_bounds = parse_bounds(value.cstring());
	m_canvas_dimensions = calculate_canvas_dimension(m_bounds, m_canvas_size);
	m_canvas_origin = calculate_canvas_origin(m_bounds, m_canvas_dimensions);
	printer().open_object("SVG bounds") << m_bounds;
	printer().close_object();

	m_point_size =
			1.0f * m_canvas_dimensions.height() * units_per_em / m_bounds.area().height() * SG_MAP_MAX / (SG_MAX);

	printer().message("Glyph count %ld", glyphs.count());


	printer().key(
				"characterSet", character_set().is_empty() ?
					"<all>" :
					character_set().cstring()
					);

	for(j=0; j < glyphs.count();j++){
		//d is the path
		JsonObject glyph = glyphs.at(j).to_object();
		printer().open_object("glyph", Printer::level_debug);
		printer() << glyph;
		printer().close_object();
		process_glyph(glyph);
	}

	for(j=0; j < hkerns.count();j++){
		//d is the path
		JsonObject hkern = hkerns.at(j).to_object();
		printer().open_object("hkern", Printer::level_debug);
		printer() << hkern;
		printer().close_object();
		process_hkern(hkern);
	}



	String output_name =
			destination_directory_path.argument() + "/" +
			FileInfo::base_name(source_file_path.argument());

	output_name
			<< String().format("-%d", m_point_size / m_downsample.height());

	m_bmp_font_generator.set_bits_per_pixel(bits_per_pixel());
	m_bmp_font_generator.set_generate_map( is_generate_map() );
	if( is_generate_map() ){
		String map_file;
		map_file << output_name << map_suffix();
		m_bmp_font_generator.set_map_output_file(map_file);
	}

	//check for missing characters
	bool is_missing = false;
	for(const auto c: character_set()){
		bool is_found = false;
		for(const auto & from_list: m_bmp_font_generator.character_list()){
			if( c == from_list.font_character().id ){
				is_found = true;
				break;
			}
		}

		if( is_found == false ){
			is_missing = true;
			printer().error("Required character %c not found", c);
		}
	}

	if( is_missing ){
		return -1;
	}

	m_bmp_font_generator.generate_font_file(output_name);
	printer().message("Created %s", output_name.cstring());
	return 0;
}

int SvgFontManager::process_hkern(const JsonObject & kerning){

	if( kerning.is_object() ){
		char first;
		char second;

		String first_string;
		String second_string;

		first_string = kerning.at("@u1").to_string();
		if( first_string.is_empty() || first_string.length() != 1 ){
			return 0;
		}

		if( character_set().find(first_string) == String::npos ){
			return 0;
		}

		second_string = kerning.at("@u2").to_string();
		if( second_string.is_empty() || second_string.length() != 1 ){
			return 0;
		}

		if( character_set().find(second_string) == String::npos ){
			return 0;
		}

		first = first_string.at(0);
		second = second_string.at(0);

		printer().message(
					"adding kerning for %c to %c",
					first, second
					);
		sg_font_kerning_pair_t kerning_pair;
		kerning_pair.unicode_first = first;
		kerning_pair.unicode_second = second;

		s16 specified_kerning = kerning.at("@k").to_integer();
		int kerning_sign;
		if( specified_kerning < 0 ){
			kerning_sign = -1;
			specified_kerning *= kerning_sign;
		} else {
			kerning_sign = 1;
		}


		//create a map to move the kerning value to bitmap distance
#if 0
		s16 mapped_kerning = specified_kerning * m_scale; //kerning value in mapped vector space -- needs to be converted to bitmap distance

		VectorMap map;
		Region region(Point(), m_canvas_dimensions);
		map.calculate_for_region(region);

		Point kerning_point(mapped_kerning, 0);

		kerning_point.map(map);

		kerning_point -= region.center();
#else
		sg_size_t mapped_kerning = map_svg_value_to_bitmap(specified_kerning);
#endif
		kerning_pair.horizontal_kerning = mapped_kerning * kerning_sign; //needs to be mapped to canvas size

		m_bmp_font_generator.kerning_pair_list().push_back(kerning_pair);

	} else {
		return -1;
	}


	return 1;
}

sg_size_t SvgFontManager::map_svg_value_to_bitmap(u32 value){

	//create a map to move the kerning value to bitmap distance
	VectorMap map;
	Region region(Point(), m_canvas_dimensions);
	map.calculate_for_region(region);

	s16 mapped_value = value * m_scale; //kerning value in mapped vector space -- needs to be converted to bitmap distance
	if( mapped_value < 0 ){
		mapped_value = 32767;
	}
	Point point(mapped_value, 0);

	point.map(map);

	point -= region.center();

	return point.x();
}

int SvgFontManager::process_glyph(const JsonObject & glyph){

	String glyph_name = glyph.at("@glyph-name").to_string();
	String unicode = glyph.at("@unicode").to_string();
	u8 ascii_value = ' ';

	bool is_in_character_set = false;
	if( !character_set().is_empty() ){
		if( unicode.length() == 1 ){
			ascii_value = unicode.at(0);
			if( character_set().find(ascii_value) != String::npos ){
				is_in_character_set = true;
			}
		} else {
			if( glyph_name == "ampersand" ){
				if( character_set().find("&") != String::npos ){
					ascii_value = '&';
					is_in_character_set = true;
				}
			}

			if( glyph_name == "quotedbl" ){
				if( character_set().find("\"") != String::npos ){
					ascii_value = '"';
					is_in_character_set = true;
				}
			}

			if( glyph_name == "quotesinglbase"  ){
				if( character_set().find("'") != String::npos ){
					ascii_value = '\'';
					is_in_character_set = true;
				}
			}

			if( glyph_name == "less" ){
				if( character_set().find("\"") != String::npos ){
					ascii_value = '<';
					is_in_character_set = true;
				}
			}

			if( glyph_name == "greater" ){
				if( character_set().find("\"") != String::npos ){
					ascii_value = '>';
					is_in_character_set = true;
				}
			}
		}
	} else {
		is_in_character_set = true;
	}

	if( is_in_character_set ){

		for(const auto & entry: m_bmp_font_generator.character_list()){
			if( entry.font_character().id == ascii_value ){
				return 0;
			}
		}

		if( !glyph_name.is_empty() ){
			printer().message("Glyph Name: %s", glyph_name.cstring());
			printer().message("Unicode is %s", unicode.cstring());
		} else {
			printer().error("Glyph name not found");
			return -1;
		}

		String x_advance = glyph.at("@horiz-adv-x").to_string();

		String drawing_path;
		drawing_path = glyph.at("@d").to_string();
		if( drawing_path.is_empty() ){
			printer().error("drawing path not found");
			return -1;
		}

		Bitmap canvas;
		canvas.set_bits_per_pixel(bits_per_pixel());

		m_vector_path_icon_list = convert_svg_path(
					canvas,
					drawing_path,
					m_canvas_dimensions,
					m_pour_grid_size,
					false
					);

#if 0
		printer().open_object("origin") << m_canvas_origin;
		printer().close_object();
		canvas.draw_line( Point(m_canvas_origin.x(), 0), Point(m_canvas_origin.x(), canvas.y_max()));
		canvas.draw_line(Point(0, m_canvas_origin.y()), Point(canvas.x_max(), m_canvas_origin.y()));

		if( m_is_show_canvas ){
			printer().open_object(String().format("character-%s", unicode.cstring()));
			printer() << canvas;
			printer().close_object();
		}
#endif


		Region active_region = canvas.calculate_active_region();
		Bitmap active_canvas(
					active_region.area(),
					Bitmap::BitsPerPixel(bits_per_pixel())
					);

		active_canvas.draw_sub_bitmap(
					sg_point(0,0),
					canvas,
					active_region
					);

		Area downsampled;
		downsampled.set_width( (active_canvas.width() + m_downsample.width()/2) / m_downsample.width() );
		downsampled.set_height( (active_canvas.height() + m_downsample.height()/2) / m_downsample.height() );
		Bitmap active_canvas_downsampled(
					downsampled,
					Bitmap::BitsPerPixel(bits_per_pixel())
					);

		active_canvas_downsampled.clear();
		active_canvas_downsampled
				.downsample_bitmap(
					active_canvas,
					m_downsample
					);

		//find region inhabited by character

		sg_font_char_t character;

		character.id = ascii_value; //unicode value
		character.advance_x = (map_svg_value_to_bitmap( x_advance.to_integer() ) + m_downsample.width()/2) / m_downsample.width(); //value from SVG file -- needs to translate to bitmap

		//derive width, height, offset_x, offset_y from image
		//for offset_x and offset_y what is the standard?

		character.width = active_canvas_downsampled.width(); //width of bitmap
		character.height = active_canvas_downsampled.height(); //height of the bitmap
		character.offset_x = (active_region.point().x() - m_canvas_origin.x() + m_downsample.width()/2) / m_downsample.width(); //x offset when drawing the character
		printer().message("offset y %d - (%d - %d)", active_region.point().y(), m_canvas_origin.y(), m_point_size);
		character.offset_y = (active_region.point().y() - (m_canvas_origin.y() - m_point_size) + m_downsample.height()/2) / m_downsample.height(); //y offset when drawing the character

		//add character to master canvas, canvas_x and canvas_y are location on the master canvas
		character.canvas_x = 0; //x location on master canvas -- set when font is generated
		character.canvas_y = 0; //y location on master canvas -- set when font is generated

		m_bmp_font_generator.character_list().push_back(
					BmpFontGeneratorCharacter(
						active_canvas_downsampled,
						character
						)
					);

#if !SHOW_ORIGIN
		if( m_is_show_canvas ){
			printer().open_object(String().format("active character-%s (%c)", unicode.cstring(), ascii_value));
			{
				printer() << active_region;
				printer() << active_canvas;
				printer() << active_canvas_downsampled;
				printer().open_object("character");
				{
					printer().key("advance x", "%d", character.advance_x);
					printer().key("offset x", "%d", character.offset_x);
					printer().key("offset y", "%d", character.offset_y);
					printer().key("width", "%d", character.width);
					printer().key("height", "%d", character.height);
					printer().close_object();
				}
				printer().close_object();
			}
		}
#endif
		if( character.id == '"' ){
			printer().info("-------------Double Quote added-----------\n");
			//exit(1);
		}

	}
	return 0;

}

void SvgFontManager::fit_icon_to_canvas(
		Bitmap & bitmap,
		VectorPath & vector_path,
		const VectorMap & map
		){
	Region active_region
			= bitmap.calculate_active_region();

	Point map_shift;
	Point bitmap_shift;
	float width_scale, height_scale;

	printer().open_object("fit icon to canvas", Printer::MESSAGE);
	printer() << bitmap;

	printer().open_object("active region");
	printer() << active_region;
	printer().close_object();

	printer().open_object("calculate offset error", Printer::DEBUG);
	printer() << bitmap.area();

	bitmap_shift = Point(
				bitmap.width()/2 - active_region.area().width()/2 - active_region.point().x(),
				bitmap.height()/2 - active_region.area().height()/2 - active_region.point().y()
				);

	width_scale = (bitmap.width()-4) * 1.0f / active_region.area().width();
	height_scale = (bitmap.height()-4) * 1.0f / active_region.area().height();

	printer().message(
				"Scaling Error is %d,%d %0.3fx%0.3f",
				bitmap_shift.x(),
				bitmap_shift.y(),
				width_scale,
				height_scale
				);

	printer().close_object();

	s32 shift_x = ((s32)bitmap_shift.x() * SG_MAP_MAX*2 + bitmap.width()/2) / bitmap.width();
	s32 shift_y = ((s32)bitmap_shift.y() * SG_MAP_MAX*2 + bitmap.height()/2) / bitmap.height() * m_scale_sign_y;

	if( m_aspect_ratio > 1.0f ){
		height_scale /= m_aspect_ratio;
	} else {
		width_scale *= m_aspect_ratio;
	}

	map_shift = Point(
				shift_x,
				shift_y
				);

	printer().open_object("vector transformation", Printer::MESSAGE);
	printer().key("scale", "%0.2fx%0.2f", width_scale, height_scale);
	printer() << map_shift;
	printer().close_object();

	vector_path.shift(map_shift);
	vector_path.scale(width_scale, height_scale);

	bitmap.clear();
	bitmap.set_pen( Pen().set_color(0xffffffff) );
	sgfx::Vector::draw(bitmap, vector_path, map);

	printer() << bitmap;

	printer().close_object();
	//bitmap.draw_rectangle(region.point(), region.area());
}


bool SvgFontManager::is_command_char(char c){

	return path_commands().find(c) != String::npos;
}

Point SvgFontManager::convert_svg_coord(float x, float y, bool is_absolute){
	float temp_x;
	float temp_y;
	sg_point_t point;


	temp_x = x;
	temp_y = y;

	//scale
	temp_x = temp_x * m_scale;
	temp_y = temp_y * m_scale * m_scale_sign_y;

	if( is_absolute ){
		//shift
		temp_x = temp_x - SG_MAP_MAX/2.0f;
		temp_y = temp_y - m_scale_sign_y * SG_MAP_MAX/2.0f;
	}

	if( temp_x > SG_MAX ){
		printer().message("Can't map this point x %0.2f > %d", temp_x, SG_MAX);
		exit(1);
	}

	if( temp_y > SG_MAX ){
		printer().message("Can't map this point y %0.2f > %d", temp_y, SG_MAX);
		exit(1);
	}

	if( temp_x < -1*SG_MAX ){
		printer().message("Can't map this point x %0.2f < %d", temp_x, -1*SG_MAX);
		exit(1);
	}

	if( temp_y < -1*SG_MAX ){
		printer().message("Can't map this point y %0.2f < %d", temp_y, -1*SG_MAX);
		exit(1);
	}


	point.x = rintf(temp_x);
	point.y = rintf(temp_y);

	return point;
}

Region SvgFontManager::parse_bounds(const String & value){
	Region result;
	Tokenizer bounds_tokens(
				value,
				Tokenizer::Delimeters(" ")
				);

	result << Point(roundf(bounds_tokens.at(0).to_float()), roundf(bounds_tokens.at(1).to_float()));
	result << Area(roundf(bounds_tokens.at(2).to_float()) - result.point().x(), roundf(bounds_tokens.at(3).to_float()) - result.point().y());

	return result;
}

Area SvgFontManager::calculate_canvas_dimension(const Region & bounds, sg_size_t canvas_size){
	float scale = 1.0f * canvas_size / bounds.area().maximum_dimension();
	return bounds.area() * scale;
}

Point SvgFontManager::calculate_canvas_origin(const Region & bounds, const Area & canvas_dimensions){
	return Point( (-1*bounds.point().x()) * canvas_dimensions.width() / (bounds.area().width()),
								(bounds.area().height() + bounds.point().y()) * canvas_dimensions.height() / (bounds.area().height()) );
}

var::Vector<FillPoint> SvgFontManager::find_fill_point_candidates(
		const Bitmap & bitmap,
		const Region & region,
		sg_size_t grid_size,
		bool is_negative_fill
		){
	var::Vector<FillPoint> result;

	Bitmap debug_bitmap(
				bitmap.area(),
				Bitmap::BitsPerPixel(bits_per_pixel())
				);
	//debug_bitmap.set_bits_per_pixel(4);

	debug_bitmap.clear();
	debug_bitmap.set_pen( Pen().set_color(1) );
	debug_bitmap.draw_bitmap(Point(0,0), bitmap);


	EdgeDetector edge_detector(bitmap);

	for(sg_int_t y = 1; y < bitmap.height(); y+=grid_size){
		//process one line at a time
		edge_detector.set_region(
					Region(
						Point(0,y),
						Area(bitmap.width(), 1)
						)
					);

		Point end_point = edge_detector.region().end_point();

		var::Vector<Array<Point, 2>> edges;
		Array<Point, 2> edge_pair;

		if( is_negative_fill ){
			edge_pair.at(0) = Point(0,0);
			edge_pair.at(1) = Point(0,0);
			edges.push_back(edge_pair);
		}

		do {
			edge_pair.at(0) = edge_detector.find_next();
			edge_pair.at(1) = edge_detector.find_next();
			if( edge_pair.at(0) != edge_detector.region().end_point() ){
				edges.push_back(edge_pair);
			}
		} while( edge_pair.at(1) != end_point );

		for(size_t i = 1; i < edges.count(); i+=2){

			/*	/0\ first edge pair
			 * /1\ second edge pair
			 * ----- fill area between even to odd pair
			 * | end of bitmap
			 *
			 * |   /0\---------/1\    /2\-------/3\    |
			 *
			 * Edge cases
			 *
			 * |  /0\---------/1\    /2\---------/3|
			 * |  /0\---------/1\    /2\           |
			 *
			 */
			//

			if( edges.at(i).at(0).x() != end_point.x() ){
				//first candidate
				result.push_back(
							FillPoint(
								Point(
									(edges.at(i).at(0).x() - edges.at(i-1).at(1).x())/2 + edges.at(i-1).at(1).x(),
									y
									),
								edges.at(i).at(0).x() - edges.at(i-1).at(1).x()
								)
							);
			}
		}
	}


	debug_bitmap.set_pen( Pen().set_color(2) );
	for(const auto & fill_point_candidate: result){
		debug_bitmap.draw_pixel(fill_point_candidate.point());
	}

	if( is_negative_fill ){
		printer().open_object("negative fill analysis", sys::Printer::DEBUG);
	} else {
		printer().open_object("fill analysis", sys::Printer::DEBUG);
	}
	printer() << debug_bitmap;
	printer().close_object();

	return result;
}


var::Vector<var::Vector<FillPoint>> SvgFontManager::group_fill_point_candidates(
		const Bitmap & bitmap,
		var::Vector<FillPoint> & fill_points
		){

	var::Vector<var::Vector<FillPoint>> result;

	int group_count = 0;
	for(u32 i=0; i < fill_points.count(); i++){
		FillPoint & fill_point = fill_points.at(i);
		if( fill_point.group() < 0 ){
			Bitmap fill_bitmap(
						bitmap.area(),
						Bitmap::BitsPerPixel(1)
						);
			Region active_region;
			Region pour_active_region;
			fill_bitmap.clear();
			fill_bitmap.set_pen( Pen().set_color(1) );
			fill_bitmap.draw_bitmap(Point(0,0), bitmap);
			active_region = bitmap.calculate_active_region();
			fill_bitmap.draw_pour(fill_point.point(), fill_bitmap.region());
			pour_active_region = bitmap.calculate_active_region();

			fill_point.set_group(group_count);
			printer().open_object(
						String().format(
							"fill %d,%d",
							fill_point.point().x(),
							fill_point.point().y()
							),
						sys::Printer::DEBUG
						);
			printer() << fill_bitmap;
			printer().close_object();

			for(size_t j = i+1; j < fill_points.count(); j++){
				FillPoint & check_point = fill_points.at(j);
				if( fill_bitmap.get_pixel(check_point.point()) != 0 ){
					check_point.set_group(group_count);
				}
			}
			group_count++;
		}
	}

	for(int group = 0; group < group_count; group++){
		var::Vector<FillPoint> group_list;
		for(const auto & fill_point: fill_points){
			if( fill_point.group() == group ){
				group_list.push_back(fill_point);
			}
		}
		result.push_back(group_list);
	}

	printer().debug("%d == %d fill point groups", result.count(), group_count);
	for(const auto & group: result){
		printer().debug("group has %d points", group.count());
	}
	return result;

}

sg_size_t SvgFontManager::get_y_fill_spacing(
		const Bitmap & bitmap,
		Point point
		){

	sg_size_t top_spacing = 0;
	sg_size_t bottom_spacing = 0;
	sg_color_t pixel;
	Point start_point = point;
	do {
		pixel = bitmap.get_pixel(point);
		bottom_spacing++;
		point += Point::Y(1);
	} while(
					(pixel == 0) && (point.y() < bitmap.height())
					);

	//hits bottom without finding a pixel
	if(( point.y() == bitmap.height()) &&
		 (pixel == 0) ){
		return 0;
	}

	point = start_point;

	do {
		pixel = bitmap.get_pixel(point);
		top_spacing++;
		point -= Point::Y(1);
	} while(
					(pixel == 0) && (point.y() > 0)
					);

	//hits top without finding a pixel
	if( (point.y() == 0) &&
			(pixel == 0) ){
		return 0;
	}

	sg_size_t spacing = top_spacing < bottom_spacing ? top_spacing : bottom_spacing;
	printer().debug(
				"point %d, %d y spacing = %d ",
				start_point.x(),
				start_point.y(),
				spacing
				);

	return spacing;
}

var::Vector<Point> SvgFontManager::find_final_fill_points(
		const Bitmap & bitmap,
		var::Vector<var::Vector<FillPoint>> & fill_point_groups,
		const var::Vector<var::Vector<FillPoint>> & negative_fill_point_groups
		){
	var::Vector<Point> result;


	PRINTER_TRACE(printer(), "figure out overlap between negative and positive groups -- mark fill group with neg group");
	for(auto & group: fill_point_groups){
		PRINTER_TRACE(printer(),
									String().format(
										"looking at group with %d points",
										group.count())
									);
		for(const auto & negative_group: negative_fill_point_groups){
			PRINTER_TRACE(printer(),
										String().format(
											"create fill bitmap with area %dx%d",
											bitmap.width(), bitmap.height()
											)
										);

			Bitmap fill_bitmap(
						bitmap.area(),
						Bitmap::BitsPerPixel(1)
						);

			PRINTER_TRACE(printer(),
										String().format(
											"looking at pour point %d,%d",
											negative_group.at(0).point().x(),
											negative_group.at(0).point().y()
											)
										);

			fill_bitmap.clear();
			PRINTER_TRACE(printer(), "draw bitmap");
			fill_bitmap.draw_bitmap(Point(0,0), bitmap);
			PRINTER_TRACE(printer(),
										String().format(
											"draw pour %d,%d in region %d,%d %dx%d -> %p",
											negative_group.at(0).point().x(),
											negative_group.at(0).point().y(),
											fill_bitmap.region().x(),
											fill_bitmap.region().y(),
											fill_bitmap.region().width(),
											fill_bitmap.region().height(),
											fill_bitmap.data()
											)
										);

			fill_bitmap.draw_pour(
						negative_group.at(0).point(),
						fill_bitmap.region()
						);

			PRINTER_TRACE(
						printer(),
						"check fill points in group"
						);
			for(auto & fill_point: group){
				if( fill_bitmap.get_pixel(fill_point.point()) != 0 ){
					printer().debug("%d:%d,%d overlaps with negative group %d (%d > %d)",
													fill_point.group(),
													fill_point.point().x(),
													fill_point.point().y(),
													negative_group.at(0).group(),
													group.count(),
													negative_group.count()
													);
					if( group.count() < negative_group.count() ){
						fill_point.set_group( -1 );
					}
				}
			}
		}
	}


	//for each group find the point with the max spacing -- need to to search y edges
	for(const auto & group: fill_point_groups){

		sg_size_t spacing = 0;
		size_t best_point = 0;

		for(size_t i=0; i < group.count(); i++){
			if( group.at(i).group() >= 0 ){
				sg_size_t x_spacing = group.at(i).spacing();
				if( x_spacing > spacing ){
					sg_size_t y_spacing = get_y_fill_spacing(bitmap, group.at(i).point());
					if( y_spacing > spacing ){
						spacing = x_spacing > y_spacing ? y_spacing : x_spacing;
						best_point = i;
					}
				}
			}
		}

		if( (spacing > 0) && (group.count() > 2) ){
			const FillPoint & fill_point = group.at(best_point);
			printer().message(
						"adding final fill point %d,%d with in spacing %d",
						fill_point.point().x(),
						fill_point.point().y(),
						spacing
						);
			result.push_back(fill_point.point());
		}
	}

	return result;
}

var::Vector<Point> SvgFontManager::find_all_fill_points(
		const Bitmap & bitmap,
		const Region & region,
		sg_size_t grid
		){

	PRINTER_TRACE(printer(), "find candidates");
	var::Vector<FillPoint> candidates
			= find_fill_point_candidates(
				bitmap,
				region,
				grid,
				false
				);

	PRINTER_TRACE(printer(), "find negative candidates");
	var::Vector<FillPoint> negative_candidates
			= find_fill_point_candidates(
				bitmap,
				region,
				grid,
				true
				);

	PRINTER_TRACE(printer(), "group candidates");
	var::Vector<var::Vector<FillPoint>> grouped_candidates
			= group_fill_point_candidates(
				bitmap,
				candidates
				);

	PRINTER_TRACE(printer(), "group negative candidates");
	var::Vector<var::Vector<FillPoint>> negative_grouped_candidates
			= group_fill_point_candidates(
				bitmap,
				negative_candidates
				);

	PRINTER_TRACE(printer(), "find final fill points");
	var::Vector<Point> fill_points = find_final_fill_points(
				bitmap,
				grouped_candidates,
				negative_grouped_candidates
				);




	return fill_points;
}

sg_size_t SvgFontManager::is_fill_point(
		const Bitmap & bitmap,
		sg_point_t point,
		const Region & region
		){

	int boundary_count;
	sg_color_t color;
	sg_point_t temp = point;

	color = bitmap.get_pixel(point);
	if( color != 0 ){
		return false;
	}

	sg_size_t width;
	sg_size_t height;

	sg_size_t spacing;

	width = region.point().x() + region.area().width();
	height = region.point().y() + region.area().height();

	boundary_count = 0;

	Cursor y_cursor(bitmap, region.point());

	do {
		Cursor x_cursor = y_cursor;

		//look for positive edge
		while( (temp.x < width) && (bitmap.get_pixel(temp) == 0) ){
			temp.x++;
		}

		spacing = temp.x - point.x;

		printer().debug(
					"-- spacing is %d vs %d",
					spacing,
					x_cursor.find_positive_edge(width)
					);

		if( temp.x < width ){
			boundary_count++;
			while( (temp.x < width) && (bitmap.get_pixel(temp) != 0) ){
				temp.x++;
			}

			if( temp.x < width ){
			}
		}
	} while( temp.x < width );

	if ((boundary_count % 2) == 0){
		return false;
	}

	temp = point;
	boundary_count = 0;
	do {
		while( (temp.x >= region.point().x()) && (bitmap.get_pixel(temp) == 0) ){
			temp.x--;
		}

		//distance to first boundary
		if( point.x - temp.x < spacing ){
			spacing = point.x - temp.x;
		}

		if( temp.x >= region.point().x() ){
			boundary_count++;
			while( (temp.x >= region.point().x()) && (bitmap.get_pixel(temp) != 0) ){
				temp.x--;
			}

			if( temp.x >= region.point().x() ){
			}
		}
	} while( temp.x >= region.point().x() );

	if ((boundary_count % 2) == 0){
		return 0;
	}

	temp = point;
	boundary_count = 0;
	do {
		while( (temp.y >= region.point().y()) && (bitmap.get_pixel(temp) == 0) ){
			temp.y--;
		}

		//distance to first boundary
		if( point.y - temp.y < spacing ){
			spacing = point.y - temp.y;
		}


		if( temp.y >= region.point().y() ){
			boundary_count++;
			while( (temp.y >= region.point().y()) && (bitmap.get_pixel(temp) != 0) ){
				temp.y--;
			}

			if( temp.y >= region.point().y() ){
			}
		}
	} while( temp.y >= region.point().y() );

	if ((boundary_count % 2) == 0){
		return 0;
	}

	//return spacing;

	temp = point;
	boundary_count = 0;
	do {
		while( (temp.y < height) && (bitmap.get_pixel(temp) == 0) ){
			temp.y++;
		}

		//distance to first boundary
		if( temp.y - point.y < spacing ){
			spacing = temp.y - point.y;
		}


		if( temp.y < height ){
			boundary_count++;
			while( (temp.y < height) && (bitmap.get_pixel(temp) != 0) ){
				temp.y++;
			}

			if( temp.y < height ){
			}
		}
	} while( temp.y < height );

	if ((boundary_count % 2) == 0){
		return 0;
	}

	return spacing;

}

var::Vector<sg_vector_path_description_t> SvgFontManager::convert_svg_path(
		Bitmap & canvas,
		const var::String & d,
		const Area & canvas_dimensions,
		sg_size_t grid_size,
		bool is_fit_icon
		){

	var::Vector<sg_vector_path_description_t> elements;

	elements = process_svg_path(d);
	if( elements.count() > 0 ){
		canvas.allocate(canvas_dimensions);

		canvas.set_pen(
					Pen().set_color(0xffffffff)
					.set_thickness(1)
					.set_fill(true)
					);

		VectorMap map;
		map.calculate_for_bitmap(canvas);
		sgfx::VectorPath vector_path;
		vector_path << elements << canvas.get_viewable_region();

		canvas.clear();
		map.set_rotation(0);
		sgfx::Vector::draw(canvas, vector_path, map);

		if( is_fit_icon ){
			printer().message("fit icon to canvas %dx%d", canvas.width(), canvas.height());
			fit_icon_to_canvas(canvas, vector_path, map);
		}

		var::Vector<Point> fill_points;
		fill_points = find_all_fill_points(canvas, canvas.get_viewable_region(), grid_size);
		printer().message(
					"found %d fill points",
					fill_points.count()
					);

		for(const auto & point: fill_points){
			Point pour_point;
			pour_point = point;
			pour_point.unmap(map);
			printer().message(
						"unmap pour point %d,%d to vector space %d,%d",
						point.x(),
						point.y(),
						pour_point.x(),
						pour_point.y()
						);

			elements.push_back(sgfx::Vector::get_path_pour(pour_point));
		}

		vector_path << elements << canvas.get_viewable_region();
		canvas.clear();
		printer().open_object("vector path", Printer::DEBUG) << vector_path;
		printer().close_object();
		canvas.set_pen( Pen().set_color(0xffffffff) );
		sgfx::Vector::draw(canvas, vector_path, map);

		canvas.set_pen(Pen().set_color(0));
		for(u32 i=0; i < fill_points.count(); i++){
			printer().message(
						"mark pour at %d,%d",
						fill_points.at(i).x(),
						fill_points.at(i).y()
						);
			canvas.draw_pixel(fill_points.at(i));
		}

		Region active_region = canvas.calculate_active_region();

		Bitmap active_bitmap(
					active_region.area(),
					Bitmap::BitsPerPixel(1)
					);
		active_bitmap.clear();
		active_bitmap.draw_sub_bitmap(Point(), canvas, active_region);
	}

	return elements;
}


var::Vector<sg_vector_path_description_t> SvgFontManager::process_svg_path(
		const String & path
		){

	String modified_path;
	String transform_path = path;
	transform_path.replace(",", String::ToInsert(" "));
	bool is_command;
	bool has_dot = false;
	for(u32 i=0; i < transform_path.length(); i++){
		is_command = path_commands_sign().find(transform_path.at(i)) != String::npos;

		if( (transform_path.at(i) == ' ') || is_command ){
			has_dot = false;
		}

		if( transform_path.at(i) == '.' ){
			if( has_dot == true ){
				modified_path << " ";
			} else {
				has_dot = true;
			}
		}

		if( (transform_path.at(i) == '-' &&
				 (path_commands().find(transform_path.at(i-1)) == String::npos)) ||
				transform_path.at(i) != '-'){
			if( is_command ){ modified_path << " "; }
		}
		//- should have a space between numbers but not commands
		modified_path << transform_path.at(i);
	}


	var::Vector<sg_vector_path_description_t> result;
	printer().debug("modified path %s", modified_path.cstring());
	Tokenizer path_tokens(
				modified_path,
				Tokenizer::Delimeters(" \n\t\r")
				);
	u32 i = 0;
	char command_char = 0;
	Point current_point, control_point;
	Point move_point;
	while(i < path_tokens.count()){
		JsonObject object;
		float arg;
		float x,y,x1,y1,x2,y2;
		Point p;
		Point points[3];
		if( is_command_char(path_tokens.at(i).at(0)) ){
			command_char = path_tokens.at(i).at(0);
			if( path_tokens.at(i).length() > 1 ){
				String scan_string;
				scan_string.format("%c%%f", command_char, &arg);
				sscanf(path_tokens.at(i++).cstring(), scan_string.cstring(), &arg);
			}
		} else {
			arg = path_tokens.at(i++).to_float();
		}
#if 0
		//used for debugging path parsing -- stop when the error shows up to pinpoint
		if( result.count() > 25 ){
			return result;
		}
#endif

		switch(command_char){
			case 'M':
				//ret = parse_path_moveto_absolute(d+i);
				x = arg;
				y = path_tokens.at(i++).to_float();
				current_point = convert_svg_coord(x, y);
				move_point = current_point;
				control_point = current_point;
				result.push_back(sgfx::Vector::get_path_move(current_point));

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'm':
				//ret = parse_path_moveto_relative(d+i);
				x = arg;
				y = path_tokens.at(i++).to_float();
				current_point += convert_svg_coord(x, y, false);
				control_point = current_point;
				move_point = current_point;
				result.push_back(sgfx::Vector::get_path_move(current_point));

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'L':
				//ret = parse_path_lineto_absolute(d+i);
				x = arg;
				y = path_tokens.at(i++).to_float();
				p = convert_svg_coord(x, y);
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'l':
				//ret = parse_path_lineto_relative(d+i);
				x = arg;
				y = path_tokens.at(i++).to_float();
				p = convert_svg_coord(x, y, false);
				p += current_point;
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'H':
				//ret = parse_path_horizontal_lineto_absolute(d+i);

				x = arg;
				p = convert_svg_coord(x, 0);
				p = Point(p.x(), current_point.y());
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'h':
				//ret = parse_path_horizontal_lineto_relative(d+i);

				x = arg;
				p = convert_svg_coord(x, 0, false);
				p = Point(p.x() + current_point.x(), current_point.y());
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'V':
				//ret = parse_path_vertical_lineto_absolute(d+i);

				y = arg;
				p = convert_svg_coord(0, y);
				p = Point(current_point.x(), p.y());
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'v':
				//ret = parse_path_vertical_lineto_relative(d+i);

				y = arg;
				p = convert_svg_coord(0, y, false);
				p = Point(current_point.x(), p.y() + current_point.y());
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'C':
				//ret = parse_path_cubic_bezier_absolute(d+i);

				x1 = arg;
				y1 = path_tokens.at(i++).to_float();
				x2 = path_tokens.at(i++).to_float();
				y2 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				points[0] = convert_svg_coord(x1, y1);
				points[1] = convert_svg_coord(x2, y2);
				points[2] = convert_svg_coord(x, y);

				result.push_back(sgfx::Vector::get_path_cubic_bezier(points[0], points[1], points[2]));
				control_point = points[1];
				current_point = points[2];


				object.insert("command", JsonInteger(command_char));
				object.insert("x1", JsonReal( arg ));
				object.insert("y1", JsonReal( y1 ));
				object.insert("x2", JsonReal( x2 ));
				object.insert("y2", JsonReal( y2 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'c':
				//ret = parse_path_cubic_bezier_relative(d+i);

				x1 = arg;
				y1 = path_tokens.at(i++).to_float();
				x2 = path_tokens.at(i++).to_float();
				y2 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				points[0] = current_point + convert_svg_coord(x1, y1, false);
				points[1] = current_point + convert_svg_coord(x2, y2, false);
				points[2] = current_point + convert_svg_coord(x, y, false);

				result.push_back(sgfx::Vector::get_path_cubic_bezier(points[0], points[1], points[2]));
				control_point = points[1];
				current_point = points[2];

				object.insert("command", JsonInteger(command_char));
				object.insert("x1", JsonReal( arg ));
				object.insert("y1", JsonReal( y1 ));
				object.insert("x2", JsonReal( x2 ));
				object.insert("y2", JsonReal( y2 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;

			case 'S':
				//ret = parse_path_cubic_bezier_short_absolute(d+i);

				x2 = arg;
				y2 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				points[0] = current_point*2 - control_point;
				//first point is a reflection of the current point
				points[0] = Point(2*current_point.x() - control_point.x(), 2*current_point.y() - control_point.y());
				points[1] = convert_svg_coord(x2, y2);
				points[2] = convert_svg_coord(x, y);

				result.push_back(sgfx::Vector::get_path_cubic_bezier(points[0], points[1], points[2]));

				control_point = points[1];
				current_point = points[2];

				object.insert("command", JsonInteger(command_char));
				object.insert("x2", JsonReal( x2 ));
				object.insert("y2", JsonReal( y2 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 's':
				//ret = parse_path_cubic_bezier_short_relative(d+i);

				x2 = arg;
				y2 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				if( current_point == control_point ){
					printer().debug("the same");
				}
				//first point is a reflection of the current point
				points[0] = Point(2*current_point.x() - control_point.x(), 2*current_point.y() - control_point.y());
				//! \todo should convert_svg_coord be relative??
				points[1] = current_point + convert_svg_coord(x2, y2, false);
				points[2] = current_point + convert_svg_coord(x, y, false);

				result.push_back(sgfx::Vector::get_path_cubic_bezier(points[0], points[1], points[2]));
				control_point = points[1];
				current_point = points[2];

				object.insert("command", JsonInteger(command_char));
				object.insert("x2", JsonReal( x2 ));
				object.insert("y2", JsonReal( y2 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'Q':
				//ret = parse_path_quadratic_bezier_absolute(d+i);

				x1 = arg;
				y1 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				points[0] = convert_svg_coord(x1, y1);
				points[1] = convert_svg_coord(x, y);

				result.push_back(sgfx::Vector::get_path_quadratic_bezier(points[0], points[1]));
				control_point = points[0];
				current_point = points[1];

				object.insert("command", JsonInteger(command_char));
				object.insert("x1", JsonReal( x1 ));
				object.insert("y1", JsonReal( y1 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'q':
				//ret = parse_path_quadratic_bezier_relative(d+i);

				x1 = arg;
				y1 = path_tokens.at(i++).to_float();
				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				points[0] = current_point + convert_svg_coord(x1, y1, false);
				points[1] = current_point + convert_svg_coord(x, y, false);

				result.push_back(sgfx::Vector::get_path_quadratic_bezier(points[0], points[1]));
				control_point = points[0];
				current_point = points[1];

				object.insert("command", JsonInteger(command_char));
				object.insert("x1", JsonReal( x1 ));
				object.insert("y1", JsonReal( y1 ));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 'T':
				//ret = parse_path_quadratic_bezier_short_absolute(d+i);

				x = arg;
				y = path_tokens.at(i++).to_float();

				points[0] = Point(2*current_point.x() - control_point.x(), 2*current_point.y() - control_point.y());
				points[1] = convert_svg_coord(x, y);

				result.push_back(sgfx::Vector::get_path_quadratic_bezier(points[0], points[1]));
				control_point = points[0];
				current_point = points[1];

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			case 't':
				//ret = parse_path_quadratic_bezier_short_relative(d+i);

				x = arg;
				y = path_tokens.at(i++).to_float();

				points[0] = current_point*2 - control_point;

				points[0] = Point(2*current_point.x() - control_point.x(), 2*current_point.y() - control_point.y());
				points[1] = current_point + convert_svg_coord(x, y, false);

				result.push_back(sgfx::Vector::get_path_quadratic_bezier(points[0], points[1]));
				control_point = points[0];
				current_point = points[1];

				object.insert("command", JsonInteger(command_char));
				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;

			case 'A':

				object.insert("command", JsonInteger(command_char));
				object.insert("rx", JsonReal( arg ));
				object.insert("ry", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("axis-rotation", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("large-arc-flag", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("sweep-flag", JsonReal( path_tokens.at(i++).to_float() ));

				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				p = convert_svg_coord(x, y);
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());


				break;

			case 'a':
				object.insert("command", JsonInteger(command_char));
				object.insert("rx", JsonReal( arg ));
				object.insert("ry", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("axis-rotation", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("large-arc-flag", JsonReal( path_tokens.at(i++).to_float() ));
				object.insert("sweep-flag", JsonReal( path_tokens.at(i++).to_float() ));

				x = path_tokens.at(i++).to_float();
				y = path_tokens.at(i++).to_float();

				p = current_point + convert_svg_coord(x, y, false);
				result.push_back(sgfx::Vector::get_path_line(p));
				current_point = p;
				control_point = current_point;

				object.insert("x", JsonReal( x ));
				object.insert("y", JsonReal( y ));
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;

			case 'Z':
			case 'z':
				//ret = parse_close_path(d+i);

				result.push_back(sgfx::Vector::get_path_close());
				current_point = move_point;
				control_point = current_point;

				object.insert("command", JsonInteger(command_char));
				i++;
				printer().debug("%c: %s", command_char, JsonDocument().set_flags(JsonDocument::COMPACT).stringify(object).cstring());
				break;
			default:
				printer().message("Unhandled command char %c", command_char);
				return result;
		}
	}

	return result;
}


