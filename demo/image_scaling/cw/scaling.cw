# This program demonstrates the management of an image scaling
# panel where the user can change either the absolute or the
# relative size of an image and the opposite values will automatically
# update.
#
# There is an option to preserve the width-height ration and when on
# changes to width will automatically change height and vice-versa.

initial_height CONSTANT(export:ro) 1500.0;
initial_width CONSTANT(export:ro) 2100.0;

absolute_size Absolute relative_size;
relative_size Relative absolute_size;
preserve_ratio FLAG(export: rw);
ratio_preserver RatioPreserver preserve_ratio, absolute_size;

# Observe changes in the relative sizes and update absolute
Relative MACHINE absolute {
  EXPORT READWRITE FLOAT32 width;
  EXPORT READWRITE FLOAT32 height;
  OPTION width 0.0;
  OPTION height 0.0;
  OPTION last_width 0.0;
  OPTION last_height 0.0;

  update_width WHEN last_width != width;
  update_height WHEN last_height != height;
  idle DEFAULT;

  ENTER update_width {
      last_width := width;
      absolute.width := width * initial_width;
  }

  ENTER update_height {
      last_height := height;
      absolute.height := height * initial_height;
  }
}

# Observer changes in the absolute sizes and update relative
Absolute MACHINE relative {
  EXPORT READWRITE FLOAT32 width;
  EXPORT READWRITE FLOAT32 height;
  OPTION width 0.0;
  OPTION height 0.0;
  OPTION last_width 0.0;
  OPTION last_height 0.0;

  update_width WHEN last_width != width;
  update_height WHEN last_height != height;
  idle DEFAULT;

  ENTER INIT {
    width := initial_width;
    height := initial_height;
  }

  ENTER update_width {
      last_width := width;
      relative.width := width / initial_width;
  }

  ENTER update_height {
      last_height := height;
      relative.height := height / initial_height;
  }
}

RatioPreserver MACHINE preserve_ratio, absolute {
  OPTION ratio 1.0;
  OPTION last_width 1.0;
  OPTION last_height 1.0;

  ENTER INIT {
    ratio := initial_width / initial_height;
    last_width := initial_width;
    last_height := initial_height;
    SET preserve_ratio TO on;
  }

  OPTION last_width 0.0;
  OPTION last_height 0.0;

  fix_height WHEN preserve_ratio IS on && last_width != absolute.width;
  fix_width WHEN preserve_ratio IS on && last_height != absolute.height;
  update_width WHEN last_width != absolute.width;
  update_height WHEN last_height != absolute.height;
  idle DEFAULT;

  ENTER fix_height {
    ratio := last_width / last_height;
    last_width := absolute.width;
    last_height := last_width / ratio;
    absolute.height := last_height;
  }
  ENTER fix_width {
    ratio := last_width / last_height;
    last_height := absolute.height;
    last_width := ratio * last_height;
    absolute.width := last_width;
  }
  ENTER update_width {
    last_width := absolute.width;
    ratio := last_width / last_height;
  }
  ENTER update_height {
    last_height := absolute.height;
    ratio := last_width / last_height;
  }
}
