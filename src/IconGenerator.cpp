#include "IconGenerator.hpp"


int IconGenerator::generate_icon_file(const String & destination){

	File font_file;
	u32 max_icon_width = 0;
	u32 max_icon_height = 0;
	u32 area;
	bool is_destination_valid = destination.is_empty() == false;

	String output_name = destination;
	if( FileInfo::suffix(destination).is_empty() ){
		output_name << ".sbi";
	}

	printer().info("Create output font file '%s'", output_name.cstring());

	if( is_destination_valid ){
		if( font_file.create(output_name, File::IsOverwrite(true)) <  0 ){
			printer().error("failed to create bmp font " + output_name);
			return -1;
		}
	}

	//populate the header
	sg_font_icon_header_t header;
	area = 0;
	header.max_height = 0;
	max_icon_height = 0;
	max_icon_width = 0;
	printer().debug("find max dimensions");
	for(const auto & icon: icon_list()){
		if( icon.bitmap().width() > max_icon_width ){
			max_icon_width = icon.bitmap().width();
		}

		if( icon.bitmap().height() > max_icon_height ){
			max_icon_height = icon.bitmap().height();
		}
	}

	printer().debug("populate header");
	header.version = SG_FONT_VERSION;
	header.bits_per_pixel = bits_per_pixel();
	header.icon_count = icon_list().count();
	header.max_height = max_icon_height;
	header.max_width = max_icon_width;
	header.resd = 0;
	header.size =
			sizeof(header) +
			header.icon_count*sizeof(sg_font_icon_t);
	header.canvas_width = (header.max_width*2*32+31)/ 32;
	header.canvas_height = header.max_height*2;

	printer().open_object("header");
	printer().key("bits_per_pixel", "%d", header.bits_per_pixel);
	printer().key("max_width", "%d", header.max_width);
	printer().key("max_height", "%d", header.max_height);
	printer().key("icon_count", "%d", header.icon_count);
	printer().key("size", "%d", header.size);
	printer().key("canvas_width", "%d", header.canvas_width);
	printer().key("canvas_heigh", "%d", header.canvas_height);
	printer().close_object();

	printer().debug("build master canvas");
	var::Vector<Bitmap> master_canvas_list = build_master_canvas(header);
	if( master_canvas_list.count() == 0 ){
		return -1;
	}

	printer().debug("show canvas");
	for(u32 i=0; i < master_canvas_list.count(); i++){
		printer().open_object(String().format("master canvas %d", i), Printer::DEBUG);
		printer() << master_canvas_list.at(i);
		printer().close_object();
	}



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

	printer().debug("sort character list");
	icon_list().sort(
				var::Vector<IconGeneratorIcon>::ascending
				);

	printer().debug(
				"write characters defs to file at %d",
				font_file.location()
				);

	//write characters in order
	//for(u32 i = 0; i < 65535; i++){
	for(const auto & icon: icon_list()){

		printer().debug("write character %s to file on %d: %d,%d %dx%d at %d",
										icon.icon().name,
										icon.icon().canvas_idx,
										icon.icon().canvas_x,
										icon.icon().canvas_y,
										icon.icon().width,
										icon.icon().height,
										font_file.seek(0, File::CURRENT));
		if( is_destination_valid &&
				font_file.write(
					icon.icon()
					) != sizeof(sg_font_icon_t) ){
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


	font_file.close();

	return 0;
}

var::Vector<Bitmap> IconGenerator::build_master_canvas(
		const sg_font_icon_header_t & header
		){
	var::Vector<Bitmap> master_canvas_list;
	Bitmap master_canvas;

	printer().debug("max width %d", header.max_width);

	Area master_dim(header.canvas_width, header.canvas_height);

	printer().debug("master width %d", master_dim.width());
	printer().debug("master height %d", master_dim.height());

	if( master_canvas.allocate(
				master_dim,
				Bitmap::BitsPerPixel(header.bits_per_pixel)
				) < 0 ){
		printer().error("Failed to allocate memory for master canvas");
		return master_canvas_list;
	}
	master_canvas.clear();


	for(auto & icon: icon_list()){
		Region region;
		Area icon_dim(
					icon.icon().width,
					icon.icon().height
					);

		printer().debug("process icon " + icon.name());
		if( icon_dim.width() && icon_dim.height() ){
			printer().debug(
						"find space for %dx%d",
						icon_dim.width(),
						icon_dim.height()
						);
			do {

				for(u32 j = 0; j < master_canvas_list.count(); j++){
					region = find_space_on_canvas(
								master_canvas_list.at(j),
								icon.bitmap().area()
								);

					if( region.is_valid() ){
						printer().debug(
									"allocate (%s) to master canvas %d at %d,%d",
									icon.icon().name,
									j,
									region.x(), region.y());
						icon.icon().canvas_x = region.x();
						icon.icon().canvas_y = region.y();
						icon.icon().canvas_idx = j;
						break;
					}

				}

				if( region.is_valid() == false ){
					printer().debug("add canvas to list");
					master_canvas_list.push_back(master_canvas);
				}

			} while( !region.is_valid() );
		}
	}

	printer().debug("draw icon on canvas");
	for(const auto & icon: icon_list()){
		Point p(
					icon.icon().canvas_x,
					icon.icon().canvas_y
					);
		printer().debug(
					"draw %s on %d",
					icon.name().cstring(),
					icon.icon().canvas_idx
					);
		master_canvas_list.at(
					icon.icon().canvas_idx
					).draw_bitmap(p, icon.bitmap());
	}

	return master_canvas_list;
}
