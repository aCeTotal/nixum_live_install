let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell { 
    hardeningDisable = ["all"];

    packages = with pkgs; [  
      gdb
      clang-tools

      # Build tools
      cmake
      pkg-config
      ninja

      # Libraries
      SDL2
      SDL2_image
      SDL2_ttf
      SDL2_net
      SDL2_gfx
      SDL2_Pango
      SDL2_sound
      SDL2_mixer
      nushell
    ];
  }
