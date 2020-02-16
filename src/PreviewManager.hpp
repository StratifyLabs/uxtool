#ifndef PREVIEWMANAGER_HPP
#define PREVIEWMANAGER_HPP

#include <sapi/sgfx.hpp>
#include "Object.hpp"

class PreviewManager : public Object
{
public:
	PreviewManager();

	void generate(
			fs::File::SourcePath source_path
			);

private:



	var::Vector<var::String> get_theme_file_names(
			const var::JsonArray & theme_array
			);

	var::Vector<var::String> get_font_file_names(
			const var::JsonObject & font_object
			);

	var::Vector<var::String> get_icon_file_names(
			const var::JsonObject & icon_object
			);

	var::Vector<var::String> get_file_names(
			const var::JsonObject & json_object
			);



	Bitmap generate_font_preview(
			const var::String & font_path
			);

	Bitmap generate_icon_preview(
			const var::String & icon_path
			);

	void generate_theme_preview(
			const var::String & theme_path,
			const var::Vector<var::String> & font_list
			);

};

#endif // PREVIEWMANAGER_HPP
