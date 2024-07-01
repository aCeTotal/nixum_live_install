
{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation {
  pname = "nixum_live_install";
  version = "1.0.0"; # Endre dette til versjonen av prosjektet ditt

  src = pkgs.fetchFromGitHub {
    owner = "aCeTotal";
    repo = "nixum_live_install";
    rev = "8255985";
    sha256 = "sha256-7BtNHU0GaFsuvlC9PBO9c136fKSRLxWqmhiNJusgQaI=";
  };

  nativeBuildInputs = [ pkgs.cmake pkgs.ninja ];

  buildInputs = [ ];

  buildPhase = ''
  rm -rf build
  mkdir build 
  cmake -G Ninja .. 
  ninja
  '';

  installPhase = ''

  '';

}
