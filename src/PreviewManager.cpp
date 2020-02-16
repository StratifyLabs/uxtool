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


	//draw all generated fonts on a bitmap
	if( font_preview_path.is_empty() == false ){
		for(const auto & file_path: font_list){
			printer().open_object("generate font preview for " + file_path);
			printer().close_object();
		}
	}

	u32 theme_count = 0;
	if( icon_preview_path.is_empty() == false ){
		var::Vector<Bitmap> font_bitmap_list;
		for(const auto & file_path: icon_list){
			printer().open_object("generate icon preview for " + file_path);
			font_bitmap_list.push_back(
						generate_font_preview(
							file_path,
							theme_list.at(theme_count++ % theme_list.count() )
							)
						);
			printer().close_object();
		}

		//put all the bitmaps on one big bitmap and then generate a BMP file

	}


	//create preview for icons



	printer().close_array();
}

Bitmap PreviewManager::generate_font_preview(
		const var::String & font_path,
		const var::String & theme_path
		){
	return Bitmap();

}

Bitmap PreviewManager::generate_icon_preview(
		const var::String & icon_path,
		const var::String & theme_path
		){

	return Bitmap();
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
