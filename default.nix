{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "nixum_install";
  version = "1.0.0";

  src = pkgs.fetchFromGitHub {
    owner = "aCeTotal";
    repo = "nixum_live_install";
    rev = "d52e735";
    sha256 = "sha256-YMmTXSW06vrndireponbGvPbxPoaSWxccFBj029bGT0=";
  };

  nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];

  buildInputs = [ pkgs.SDL2 pkgs.SDL2_ttf ];

  configurePhase = ''
    rm -rf CMakeCache.txt CMakeFiles
    cmake -G Ninja .
  '';

  buildPhase = ''
    ninja
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp -r nixum_install test.ttf $out/bin
    ./result/bin/nixum_install
        '';
}
