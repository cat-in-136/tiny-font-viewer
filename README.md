tiny-font-viewer
================

A lightweight GTK4 font file viewer

# About

tiny-font-viewer is a fork of gnome-font-viewer, stripped down to its essentials.
It is a simple utility tool for viewing individual font files,
without the overhead of listing all installed fonts on the system.

# Features

 * Show sample text rendering and font information
 * Supports viewing of individual font files (TTF, OTF, etc.)
 * Supports font collections (TTC)
 * Built using GTK4, without dependencies on GNOME or libadwaita

# Usage

 * Using the GUI: Open tiny-font-viewer, then select File > Open from the menu
   and choose the font file you want to view.
 * From the command line: Execute `tiny-font-viewer FONTFILE`,
 replacing FONTFILE with the path to the font file you want to view.
 * Using the desktop file: If you have installed
   `io.github.cat-in-136.tiny-font-viewer.desktop` file,
   you can also open a font file by double-clicking on it in your file viewer,
   or by running `xdg-open FONTFILE`.

# Building and Installation

To build and install tiny-font-viewer, follow these steps:

 * Clone the repository:
   `git clone https://github.com/cat-in-136/tiny-font-viewer.git`
 * Build the application: `meson build && ninja -C build`
 * Install the application: `ninja -C build install`

# License

tiny-font-viewer is licensed under the GPLv2+ license.

# Acknowledgments

This project is a fork of gnome-font-viewer,
and gratefully acknowledges the work of the GNOME project and its contributors.

