#include <sapi/sgfx/Font.hpp>

#include "Options.hpp"

Options::Options(const Cli& cli)
{

	m_action = cli.get_option(
				"action",
				Cli::Description("specify the action to perform --action=show|convert|clean|process")
				);

	m_help = cli.get_option(
				"help",
				Cli::Description("show help options")
				);

	String hint_string = cli.get_option(
				"hint",
				Cli::Description("gives uxtool additional hints --hint=map,json,details,theme,icon,printer")
				);

	m_hint_list = hint_string.split(",");

	m_is_map = m_hint_list.find("map") < m_hint_list.count();
	m_is_json = m_hint_list.find("json") < m_hint_list.count();
	m_is_details = m_hint_list.find("details") < m_hint_list.count();
	m_is_theme = m_hint_list.find("theme") < m_hint_list.count();
	m_is_icon = m_hint_list.find("icon") < m_hint_list.count();
	m_is_json_printer = m_hint_list.find("printer") < m_hint_list.count();

	m_input = cli.get_option(
				"input",
				Cli::Description("specify the path to the input --input=<path> (file suffix should be sbf|svg|bmp|txt)")
				);

	m_output = cli.get_option(
				"output",
				Cli::Description("specify the path to the output folder --output=<path>")
				);

	m_point_size = cli.get_option(
				"size",
				Cli::Description("specify the canvas size to use in pixels --size=128")
				);

	if( m_point_size.to_integer() == 0 ){
		m_point_size = "128";
	}

	m_pour_size = cli.get_option(
				"pour",
				Cli::Description("specify the size of the grid pour search --pour=4")
				);

	if( m_pour_size.to_integer() == 0 ){
		m_pour_size = "3";
	}

	m_downsample_size = cli.get_option(
				"downsample",
				Cli::Description("specify the downsampling factor --downsampling=4")
				);

	if( m_downsample_size.to_integer() == 0 ){
		m_downsample_size = "4";
	}

	m_is_overwrite = cli.get_option(
				"overwrite",
				Cli::Description("overwrite existing files --overwrite=true")
				) == "true";


	m_characters = cli.get_option(
				"characters",
				Cli::Description("specify the characters to process (default is ascii)")
				);

	if( m_characters.is_empty() ){
		m_characters = sgfx::Font::ascii_character_set();
		m_characters.erase(String::Position(0), String::Length(1));
	}

	m_bits_per_pixel = cli.get_option(
				"bpp",
				Cli::Description("specify the number of bits to use for each pixel --bpp=<1|2|4|8>")
				);

	if( m_bits_per_pixel.to_integer() == 0 ){
		m_bits_per_pixel = "1";
	}

	switch(m_bits_per_pixel.to_integer()){
		case 1:
		case 2:
		case 4:
		case 8:
			break;
		default:
			exit(0);
	}

	m_input_suffix = fs::FileInfo::suffix(m_input);
	m_output_suffix = fs::FileInfo::suffix(m_output);

}

void Options::show(){
	PrinterObject og(printer(), "options");
	printer().key("action", action());
	if( action() != "process" ){
		printer().key("details", is_details() ? "true" : "false");
		printer().key("input", input());
		printer().key("output", output().is_empty() ? "<auto>" : output().cstring() );
		printer().key("size", point_size());
		printer().key("downsample", downsample_size());
		printer().key("pour", pour_size());
		printer().key("overwrite", is_overwrite() ? "true" : "false");
		printer().key("characters", characters().is_empty() ? "<ascii>" : characters().cstring() );
		printer().key("bitsPerPixel", bits_per_pixel());
		printer().key("json", is_json() ? "true" : "false");
	}
}
