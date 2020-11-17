#!/bin/bash

# This script combines images in data/base/images/src/ to create the icons for the reticule buttons.

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

source_dir="$DIR/../../data/base/images/src"
target_dir="$DIR/../../data/base/images/intfac"
scale=2
border_thickness=scale=$((scale * 1))
small_width=$((border_thickness * 38))
small_height=$((border_thickness * 30))
big_width=$((border_thickness * 46))
big_height=$((border_thickness * 38))

main () {
    for i in build research design intelmap manufacture commanddroid
    do
        generate_reticule_icon $i $((small_width)) $((small_height))
    done

    generate_reticule_icon cancel $((big_width)) $((big_height))

    convert $(border $((small_width + 2 * scale)) $((small_height + 2 * scale)) 3) $target_dir/image_reticule_hilight.png &
    convert $(border $((big_width + 2 * scale)) $((big_height + 2 * scale)) 3) $target_dir/image_cancel_hilight.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $((small_width)) $((small_height))) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 2 * scale)) $((small_height - 2 * scale))) \
            -modulate 50,0 \
        \) \
        -gravity center \
        -compose over \
        -composite \
        $target_dir/image_reticule_grey.png &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" $((small_width)) $((small_height))) \
        \( \
            $(rasterized_svg "$source_dir/shape.svg" $((small_width - 2 * scale)) $((small_height - 2 * scale))) \
            -modulate 200 \
        \) \
        -gravity center \
        -compose over \
        -composite \
        $target_dir/image_reticule_butdown.png &

    wait
}

rasterized_svg () {
    image="$1"
    width="$2"
    height="$3"
    echo "( -density 1200 -background none $image -resize ${width}x${height}! )"
}

border () {
    width="$1"
    height="$2"
    thickness="$3"

    echo "(
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height")
        (
            $(rasterized_svg "$source_dir/shape.svg" $((width - scale * thickness)) $((height - scale * thickness)))
            -channel a -negate +channel
        )
        -gravity center
        -channel A
        -compose multiply
        -composite +channel
    )"
}

generate_reticule_icon () {
    icon_name="$1"
    width="$2"
    height="$3"

    convert \
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height") \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * border_thickness)) $((height - 2 * border_thickness))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        -layers merge \
        "$target_dir/image_${icon_name}_up.png" &

    convert \
        $(rasterized_svg "$source_dir/shape.svg" "$width" "$height") \
        \( \
            $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            $(rasterized_svg "$source_dir/shape.svg" $((width - 2 * border_thickness)) $((height - 2 * border_thickness))) \
            -gravity center \
            -compose copyopacity \
            -composite \
        \) \
        \( \
            \( \
                -modulate 150,300 \
                $(rasterized_svg "$source_dir/$icon_name.svg" "$width" "$height") \
            \) \
            \( \
                $(rasterized_svg "$source_dir/glow_mask.svg" $((width - 4 * scale)) $((height - 4 * scale))) \
                $(rasterized_svg "$source_dir/shape.svg" $((width - 4 * scale)) $((height - 4 * scale))) \
                -gravity center \
                -channel A \
                -compose multiply \
                -composite +channel \
            \) \
            -compose copyopacity \
            -composite \
        \) \
        -compose over \
        +flatten \
        "$target_dir/image_${icon_name}_down.png" &
}

main
