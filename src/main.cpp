/*! \file */ //Copyright 2011-2018 Tyler Gilbert; All Rights Reserved

#include <sapi/fmt.hpp>
#include <sapi/hal.hpp>
#include <sapi/sys.hpp>
#include <sapi/var.hpp>
#include <sapi/sgfx.hpp>

#include "Util.hpp"
#include "SvgFontManager.hpp"
#include "ConvertManager.hpp"
#include "ThemeManager.hpp"
#include "PreviewManager.hpp"
#include "ApplicationPrinter.hpp"
#include "Options.hpp"

void process_batch(const var::String & path);
void show_usage(const Cli & cli);

int main(int argc, char * argv[]){



	Cli cli(argc, argv);
	cli.set_publisher("Stratify Labs, Inc");
	cli.handle_version();
	Options options(cli);
	Ap::printer().set_verbose_level( cli.get_option("verbose") );

	if( options.is_help() == false ){
		options.show();
	}

	if( options.action() == "process" ){
		process_batch(options.input());
		exit(0);
	}

	if( options.action() == "show" ){
		Util::show(options);
		exit(0);
	}

	if( options.action() == "clean" ){
		Ap::printer().message(
					"Cleaning directory %s from sbf files",
					options.input().cstring()
					);
		Util::clean_path(options.input(), "sbf");
		exit(0);
	}

	if( options.action() == "convert" ){
		Ap::printer().message("converting");
		//input is a bmp, svg or map file
		if( options.input().is_empty() || options.input() == "true" ){
			Ap::printer().error("input file or directory must be specified with --input=<path>");
			return 1;
		}

		if( Util::convert(options) == true ){
			exit(0);
		}

		ConvertManager convert_manager;
		if( convert_manager.convert(options) == true ){
			exit(0);
		}

		ThemeManager theme_manager;
		if( theme_manager.convert(options) == true ){
			exit(0);
		}


		BmpFontGenerator bmp_font_generator;
		if( bmp_font_generator.convert(options) == true ){
			exit(0);
		}


	}

	if( cli.get_option("help") == "true" ){
		show_usage(cli);
		exit(0);
	}

	return 0;
}

void show_usage(const Cli & cli){
	cli.show_options();
}

void process_batch(const var::String & path){
	String input_json_path = path;
	if( input_json_path.is_empty() ){
		input_json_path = "ux_settings.json";
	}

	JsonObject input_object = JsonDocument().load(
				JsonDocument::FilePath(input_json_path)
				).to_object();

	u8 bits_per_pixel = input_object.at("bitsPerPixel").to_integer();
	if( bits_per_pixel == 0 ){
		bits_per_pixel = 1;
	}


	String output_path = input_object.at("output").to_string();
	if( output_path.is_empty() ){
		Ap::printer().error("`output` not present for output path");
		return;
	}

	JsonObject fonts = input_object.at("fonts").to_object();
	String characters = fonts.at("characters").to_string();
	if( characters.is_empty() ){
		characters = Font::ascii_character_set();
		characters.erase(String::Position(0), String::Length(1));
	}
	var::Vector<var::String> font_input_list = fonts.at("paths").to_array().string_list();
	var::Vector<s32> font_size_list = fonts.at("sizes").to_array().integer_list();


	Ap::printer().open_object("process");
	Ap::printer().key("bitsPerPixel", "%d", bits_per_pixel);
	Ap::printer().key("output", output_path);
	Ap::printer().key("characters", characters);
	Ap::printer().close_object();

#if 1
	ConvertManager convert_manager;
	convert_manager.set_character_set(characters);
	convert_manager.set_bits_per_pixel(bits_per_pixel);
	convert_manager.set_generate_map(false);

	for(const auto & path: font_input_list){
		for(const auto point_size: font_size_list){
			convert_manager.set_point_size( point_size );
			convert_manager.process_font(
						File::SourcePath(path),
						File::DestinationPath(output_path)
						);
		}
	}

	JsonObject icons = input_object.at("icons").to_object();
	var::Vector<var::String> icon_input_list = icons.at("paths").to_array().string_list();
	var::Vector<s32> icon_size_list = icons.at("sizes").to_array().integer_list();

	for(const auto & path: icon_input_list){
		for(const auto point_size: icon_size_list){
			convert_manager.set_point_size( point_size );
			convert_manager.process_icons(
						File::SourcePath(path),
						File::DestinationPath(output_path)
						);
		}
	}
#endif

	ThemeManager theme_manager;

	theme_manager.import(
				fs::File::SourcePath(input_json_path),
				fs::File::DestinationPath(output_path)
				);

	PreviewManager preview_manager;

	preview_manager.generate(
				fs::File::SourcePath(input_json_path)
				);

}

