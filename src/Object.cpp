#include "Object.hpp"

Object::Object(){
	m_bits_per_pixel = 1;
	m_is_generate_map = false;
	m_character_set = Font::ascii_character_set();
	m_is_ascii = true;
}


Region Object::find_space_on_canvas(Bitmap & canvas, Area dimensions){
	Region region;
	sg_point_t point;

	region << dimensions;

	for(point.y = 0; point.y < canvas.height() - dimensions.height(); point.y++){
		for(point.x = 0; point.x < canvas.width() - dimensions.width(); point.x++){
			region << point;
			if( canvas.is_empty(region) ){
				canvas.draw_rectangle(region.point(), region.area());
				return region;
			}
		}
	}

	return Region();
}
