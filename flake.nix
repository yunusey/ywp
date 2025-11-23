{
  description = "Flake to manage ywp dependencies";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem
    (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
        libs = with pkgs; [
          wayland
          wayland-scanner
          wayland-protocols
          libffi
          libGLU
          egl-wayland
          libxkbcommon
          fftw
          pulseaudio
        ];
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "ywp";
          version = "1.0.0";

          src = ./.;

          buildInputs = libs;
          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.cmake
            pkgs.unixtools.xxd
          ];

          meta = with pkgs.lib; {
            description = "ywp - Custom wallpaper manager written in C.";
            license = licenses.mit;
            maintainers = with maintainers; [yunusey];
            platforms = platforms.linux;
          };
        };
        devShells.default = pkgs.mkShell {
          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath libs;
          CMAKE_PREFIX_PATH = pkgs.lib.makeLibraryPath libs;
          buildInputs = libs;
          nativeBuildInputs =
            [
              pkgs.pkg-config
              pkgs.cmake
            ]
            ++ libs;
        };
      }
    );
}
