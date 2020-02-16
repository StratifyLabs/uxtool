#include "PreviewManager.hpp"

PreviewManager::PreviewManager(){

}


void PreviewManager::generate(
		fs::File::SourcePath source_path
		){


	printer().open_array("generate preview");
	JsonObject configuration = JsonDocument().load(
				fs::File::Path(source_path.argument())
				).to_object();

	u8 bpp = configuration.at("bitsPerPixel").to_integer();

	String font_preview_path = configuration.at("fontPreview").to_string();
	if( font_preview_path.is_empty() == false ){
		printer().info("font preview is " + font_preview_path);
	}

	String icon_preview_path = configuration.at("iconPreview").to_string();
	if( icon_preview_path.is_empty() == false ){
		printer().info("icon preview is " + icon_preview_path);
	}

	String theme_preview_path = configuration.at("themePreview").to_string();
	if( theme_preview_path.is_empty() == false ){
		printer().info("theme preview is " + theme_preview_path);
	}

	String asset_folder =
			configuration.at("output").to_string();

	var::Vector<var::String> font_list;
	var::Vector<var::String> icon_list;
	var::Vector<var::String> theme_list;

	font_list = get_font_file_names(
				configuration.at("fonts").to_object()
				);

	icon_list = get_icon_file_names(
				configuration.at("icons").to_object()
				);

	theme_list = get_theme_file_names(
				configuration.at("themes").to_array()
				);

	for(auto & entry: font_list ){ entry = asset_folder + "/" + entry; }
	for(auto & entry: icon_list ){ entry = asset_folder + "/" + entry; }
	for(auto & entry: theme_list ){ entry = asset_folder + "/" + entry; }

	for(const auto & file_path: theme_list){
		printer().info("detected theme " + file_path);
	}


	u32 theme_count = 0;
	//draw all generated fonts on a bitmap
	if( font_preview_path.is_empty() == false ){
		var::Vector<Bitmap> bitmap_list;
		for(const auto & file_path: font_list){
			printer().open_object("generate font preview for " + file_path);
			bitmap_list.push_back(
						generate_font_preview(
							file_path
							)
						);
			printer().close_object();
		}

		//put all the bitmaps on one big bitmap and then generate a BMP file
		sg_size_t max_width = 0;
		sg_size_t total_height = 0;

		for(const auto & bitmap: bitmap_list){
			total_height += bitmap.height();
			if( bitmap.width() > max_width ){
				max_width = bitmap.width();
			}
		}

		Bitmap all_fonts_bitmap(
					Area(max_width, total_height),
					Bitmap::BitsPerPixel(4)
					);

		u32 y_cursor = 0;
		printer().message("draw fonts on single bitmap");
		for(const auto & bitmap: bitmap_list){
			all_fonts_bitmap.draw_bitmap(
						Point(0, y_cursor),
						bitmap
						);

			y_cursor += bitmap.height();
		}


		Palette palette;
		palette
				.set_pixel_format(Palette::pixel_format_rgb888)
				.set_color_count(Palette::get_color_count(bpp))
				.create_gradient(PaletteColor("#ffffff"));

		printer().open_array("bmp palette", Printer::level_message);
		for(u32 i=0; i < palette.colors().count(); i++){
			printer().message("color is " + palette.palette_color(i).to_hex_code());
		}
		printer().close_array();

		printer().message("save bitmap " + font_preview_path);

		Bmp::save(
					font_preview_path,
					all_fonts_bitmap,
					palette
					);
		printer().message("done saving " + font_preview_path);
	}

	//create preview for icons

	if( icon_preview_path.is_empty() == false ){
		var::Vector<Bitmap> bitmap_list;
		for(const auto & file_path: icon_list){
			printer().open_object("generate icon preview for " + file_path);
			bitmap_list.push_back(
						generate_icon_preview(
							file_path
							)
						);
			printer().close_object();
		}

		//put all the bitmaps on one big bitmap and then generate a BMP file
		sg_size_t max_width = 0;
		sg_size_t total_height = 0;

		for(const auto & bitmap: bitmap_list){
			total_height += bitmap.height();
			if( bitmap.width() > max_width ){
				max_width = bitmap.width();
			}
		}

		Bitmap all_fonts_bitmap(
					Area(max_width, total_height),
					Bitmap::BitsPerPixel(4)
					);

		u32 y_cursor = 0;
		printer().message("draw icons on single bitmap");
		for(const auto & bitmap: bitmap_list){
			all_fonts_bitmap.draw_bitmap(
						Point(0, y_cursor),
						bitmap
						);

			y_cursor += bitmap.height();
		}


		Palette palette;
		palette
				.set_pixel_format(Palette::pixel_format_rgb888)
				.set_color_count(Palette::get_color_count(bpp))
				.create_gradient(PaletteColor("#ffffff"));

		printer().open_array("bmp palette", Printer::level_message);
		for(u32 i=0; i < palette.colors().count(); i++){
			printer().message("color is " + palette.palette_color(i).to_hex_code());
		}
		printer().close_array();

		printer().message("save bitmap " + icon_preview_path);

		Bmp::save(
					icon_preview_path,
					all_fonts_bitmap,
					palette
					);
		printer().message("done saving " + icon_preview_path);
	}

	if( theme_preview_path.is_empty() == false ){
		//generate theme preview
		for(const auto & file_path: theme_list){
		generate_theme_preview(
					file_path,
					font_list
					);

		}

	}


	printer().close_array();
}

Bitmap PreviewManager::generate_font_preview(
		const var::String & font_path
		){

	String characters = Font::ascii_character_set();

	FontInfo font_info(font_path);

	font_info.create_font();

	Bitmap canvas(
				Area(
					font_info.font()->calculate_length(characters),
					font_info.font()->get_height() + 4
					),
				Bitmap::BitsPerPixel(font_info.font()->bits_per_pixel())
				);

	font_info.font()->draw(
				characters,
				canvas,
				Point(0, 2)
				);


	return canvas;
}

Bitmap PreviewManager::generate_icon_preview(
		const var::String & icon_path
		){

	IconFontInfo icon_font_info(icon_path);

	icon_font_info.create_font();


	Bitmap canvas(
				Area(
					(icon_font_info.point_size()+4) * icon_font_info.icon_font()->count(),
					icon_font_info.point_size() + 4
					),
				Bitmap::BitsPerPixel(icon_font_info.icon_font()->bits_per_pixel())
				);

	sg_int_t x_cursor = 0;
	for(u32 i=0; i < icon_font_info.icon_font()->count(); i++){
		icon_font_info.icon_font()->draw(
					i,
					canvas,
					Point(x_cursor+2, 2)
					);
		x_cursor += icon_font_info.point_size()+4;
	}

	return canvas;

}

void PreviewManager::generate_theme_preview(
		const var::String & theme_path,
		const var::Vector<var::String> & font_list
		){

	File theme_file;

	Theme theme(theme_file);

	theme.load(theme_path);
	u32 font_idx = 0;
	for(u8 style = 0; style < Theme::last_style; style++){

		for(u8 state = 0; state < Theme::last_state; state++){

			FontInfo font_info(font_list.at(font_idx++ % font_list.count()));

			font_info.create_font();
			enum Theme::style official_style =
					static_cast<enum Theme::style>(style);
			enum Theme::state official_state =
					static_cast<enum Theme::state>(state);

			String label =
					Theme::get_style_name(official_style) + " " +
					Theme::get_state_name(official_state);

			Area area(
						font_info.font()->calculate_length(
							label
							) + 16,
						font_info.point_size()*2
						);

			printer().message("theme has " +
										 String::number(theme.bits_per_pixel()) +
										 " bits per pixel");
			Bitmap canvas(
						area,
						Bitmap::BitsPerPixel(theme.bits_per_pixel())
						);

			canvas << Pen().set_color( theme.background_color() );

			canvas.draw_rectangle(
						canvas.region()
						);

			canvas << Pen().set_color( theme.border_color() );

			canvas.draw_rectangle(
						Point(2,2),
						Area(canvas.width()-4, canvas.height()-4)
						);

			canvas << Pen().set_color( theme.color() );

			canvas.draw_rectangle(
						Point(4,4),
						Area(canvas.width()-8, canvas.height()-8)
						);


			canvas << Pen()
								.set_color( theme.text_color() )
								.set_zero_transparent();

			font_info.font()->draw(
						label,
						canvas,
						Point(8,8)
						);

			Palette p;
			p
					.set_pixel_format(Palette::pixel_format_rgb565)
					.set_color_count(Palette::get_color_count(theme.bits_per_pixel()))
					.create_gradient(PaletteColor("#ffffff"));

			printer().info("create " + label);
			Bmp::save(
						"tmp/" + label + ".bmp",
						canvas,
						theme.palette(official_style, official_state)
						);

			//printer() << canvas;

			//exit(1);
		}
	}
}

var::Vector<var::String> PreviewManager::get_theme_file_names(
		const var::JsonArray & theme_array
		){
	var::Vector<var::String> result;
	for(u32 i=0; i < theme_array.count(); i++){
		result.push_back(
					theme_array.at(i).to_object().at("name").to_string() + ".sth"
					);
	}
	return result;
}

var::Vector<var::String> PreviewManager::get_font_file_names(
		const var::JsonObject & font_object
		){
	var::Vector<var::String> result;
	result = get_file_names(font_object);
	for(auto & entry: result){
		entry += ".sbf";
	}
	return result;
}

var::Vector<var::String> PreviewManager::get_icon_file_names(
		const var::JsonObject & icon_object
		){
	var::Vector<var::String> result;
	result = get_file_names(icon_object);
	for(auto & entry: result){
		entry += ".sbi";
	}
	return result;
}

var::Vector<var::String> PreviewManager::get_file_names(
		const var::JsonObject & json_object
		){

	var::Vector<var::String> result;
	var::Vector<var::String> sizes =
			json_object.at("sizes").to_array().string_list();

	var::Vector<var::String> paths =
			json_object.at("paths").to_array().string_list();

	for(const auto & path: paths){
		for(const auto & size: sizes){
			result.push_back(
						FileInfo::base_name(path) + "-" + size);
		}
	}

	return result;
}
