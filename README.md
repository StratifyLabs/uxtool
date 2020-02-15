# fonttool

Font tool is a desktop tool used to convert vector fonts and icons to bitmap or vectors fonts and icons that can be used with Stratify OS.

# Input Files

Input files can be 

- the output of the GlyphDesigner (bmp and txt files)
- json files describing svg data

TTF fonts can be converted to svg using an online converter. The resulting svg's can be converted to json using the svgson node program.

# Viewing Fonts and Icons

Use `--action=show` to view fonts and icons.

```
fonttool --action=show --input=assets/opensansc-l-15.sbf
fonttool --action=show --input=assets/icons.svic
```

# Creating fonts and Icons

## Icons

Convert the svg files in the specified folder to a Stratify OS vector icon format:

```
fonttool --action=convert --icon --input=icons --output=assets/icons.svic
```

Or convert a single icon:

```
fonttool --action=convert --icon --input=icons/adjust-solid.svg --output=assets/adjust-solid.svic
```

![Font Awesome Regular](examples/regular.jpg)

## Fonts

Font input files must be in svg format. TTF files can be converted to SVG using online font file converters such as https://convertio.co/ttf-svg/.

Once you have a svg font file use:

```
fonttool --action=convert --input=fonts/opensansc-l.svg --output=assets
```

![Open Sans 40pt](examples/opensansc-l-40.jpg)

### Font Maps

When generating a font, you can generate a map file that is human editable to fine tune the font.

```
fonttool --action=convert --input=fonts/opensansc-l.svg --output=assets --map
```

Once the editing is done, you can generate a font from the map file.

```
fonttool --action=convert --input=assets/opensansc-l-15-map.json --output=assets
fonttool --action=show --input=assets/opensansc-l-15-map.sbf
```

# File Formats

Both file formats are based on the (Stratify Graphics Library)[https://github.com/StratifyLabs/sgfx].  

## Stratify Bitmap Font (sbf)

The sbf format is:

```
sg_font_header_t header;
sg_font_kerning_pair_t kerning_pairs[];
sg_font_character_t characters[];
<master bitmaps>
```

The header includes:

- number of kerning pairs
- number of characters
- size and number of master bitmaps
- number of bits per pixel (1,2,4 or 8)

More information is available with in the (Stratify Graphics Library)[https://github.com/StratifyLabs/sgfx].

## Stratify Bitmap Icon Collection (sbic)

The sbic format shown below.

```
sg_bitmap_icon_header_t header;
sg_bitmap_icon_entry_t icon[];
<master canvas bitmaps>
```

The header shows

- version
- number of bits per pixel
- number of icons and size (all icons are the same size)
- master canvas count and size

## Stratify Vector Icon Collection (svic)

The svic format contains a list of stratify grapics vectors. The vectors must be rendered in real or pseudo-real time (they can't just be copied like bitmap formats).

The format is a list of vector headers and the vector paths:

```
sg_vector_icon_header_t header0;
sg_vector_path_description_t vector_paths0[];
sg_vector_icon_header_t header1;
sg_vector_path_description_t vector_paths0[];
...
sg_vector_icon_header_t headerN;
sg_vector_path_description_t vector_pathsN[];
```

The header defines:

- name of the icon
- number of vector paths in the icon
- location of the vector paths in the file


# Testing Shortcuts

Icons:

```
fonttool --action=convert --icon --input=icons/svgs/regular --output=assets --canvas=128
fonttool --action=show --input=assets/regular.svic --output=assets/regular.bmp
fonttool --action=convert --icon --input=icons/svgs/solid/adjust.svg --output=assets/adjust-solid.svic
fonttool --action=show --input=assets/adjust-solid.svic
fonttool --action=show --input=assets/adjust-solid.svic --canvas=32
fonttool --action=show --input=assets/regular.svic --canvas=128 --downsample=4

```

Fonts:


```
fonttool --action=convert --input=fonts/opensansc-l.svg --output=assets --map
fonttool --action=show --input=assets/opensansc-l-15.sbf
fonttool --action=convert --input=assets/opensansc-l-15-map.txt --output=assets
```
